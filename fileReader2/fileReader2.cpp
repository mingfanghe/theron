#include <vector>
#include <queue>
#include <Theron/Theron.h>

using namespace std;

template <class WorkMessage>
class Worker : public Theron::Actor
{
public:
	// 构造函数 
	Worker(Theron::Framework &framework) : Theron::Actor(framework)
	{
		RegisterHandler(this, &Worker::Handler);
	}
private:
	// 消息处理函数 
	void Handler(const WorkMessage &message, const Theron::Address from)

	{
		// 消息参数是const，搜易我们需要拷贝它来改变它的性质 
		WorkMessage result(message);
		result.Process();
		// 返回线程消息给发送者
		Send(result, from);
	}
};

//数据读取请求：读取一个磁盘文件的内容到缓存区
struct ReadRequest {
public:
	explicit ReadRequest(
		const Theron::Address client = Theron::Address(),
		const char *const fileName = 0,
		unsigned char *const buffer = 0,
		const unsigned int bufferSize = 0) :
		mClient(client),
		mFileName(fileName),
		mBuffer(buffer),
		mBufferSize(bufferSize),
		mFileSize(0)
	{
	}

	void Process()
	{
		mProcessed = true;
		mFileName = 0;
		// 尝试打开文件 
		FILE *const handle = fopen(mFileName, "rb");
		if (handle != 0)
		{
			// 读取数据文件，设置实际读取长度 
			mFileSize = (uint32_t)fread(
				mBuffer,
				sizeof(unsigned char),
				mBufferSize,
				handle);

			fclose(handle);
		}
	}

	Theron::Address mClient;            // 请求客户的地址
	const char *mFileName;              // 请求文件的名称 
	bool mProcessed;                    // 文件是否被读取过
	unsigned char *mBuffer;             // 文件内容的缓存区
	unsigned int mBufferSize;           // 缓存区的大小
	unsigned int mFileSize;             // 文件的字节长度 
};

// 一个派遣actor处理工作事例
// 派遣actor会在内部创建和控制一系列workers. 
// 它协调workers并行处理工作事例
template <class WorkMessage>
class Dispatcher : public Theron::Actor
{
public:
	Dispatcher(Theron::Framework &framework, const int workerCount) :Theron::Actor(framework)
	{
		// 创建workers并且将它们加入空闲的列中
		for (int i = 0; i < workerCount; ++i)
		{
			//Workers群体是使用new运算符在构造器中被动态创建的，储存在一个指针向量中，并且是在析构函数中使用delete运算符被销毁。
			mWorkers.push_back(new WorkerType(framework));
			mFreeQueue.push(mWorkers.back()->GetAddress());
		}
		RegisterHandler(this, &Dispatcher::Handler);
	}

	~Dispatcher()
	{
		//释放workers
		const int workerCount(static_cast<int>(mWorkers.size()));
		for (int i = 0; i < workerCount; ++i)
		{
			delete mWorkers[i];
		}
	}

private:
	typedef Worker<WorkMessage> WorkerType;

	// 从客户端处理读取请求
	void Handler(const WorkMessage &message, const Theron::Address from)
	{
		// 是否这个工作事例被处理过？
		if (message.mProcessed)
		{
			// 发送结果返回给请求的调用者 
			Send(message, message.mClient);

			// 将worker加入到空闲的列中
			mFreeQueue.push(from);
		}
		else
		{
			// 将没有处理的工作事例放入工作列中
			mWorkQueue.push(message);
		}

		// 服务于工作队列
		while (!mWorkQueue.empty() && !mFreeQueue.empty())
		{
			Send(mWorkQueue.front(), mFreeQueue.front());
			mFreeQueue.pop();
			mWorkQueue.pop();
		}
	}

	std::vector<WorkerType *> mWorkers; // 拥有的workers的指针
	std::queue<Theron::Address> mFreeQueue; // 空闲workers队列.
	std::queue<WorkMessage> mWorkQueue; // 未处理工作消息队列
};

static const int MAX_FILES = 16;
static const int MAX_FILE_SIZE = 16384;

int main(int argc, char *argv[])
{
	//Theron::Framework对象可以被有参构造
	//创建一个拥有n个线程的framework，n为文件的数量，目的：可以并行读取所有的文件
	//限制工作线程为前两个处理器核
	Theron::Framework::Parameters frameworkParams;
	frameworkParams.mThreadCount = MAX_FILES;
	frameworkParams.mProcessorMask = (1UL << 0) | (1UL << 1);
	Theron::Framework framework(frameworkParams);

	if (argc < 2)
	{
		printf("Excepted up to 16 file name arguments.\n");
	}

	//注册一个receive和一个处理函数，用来捕获结果消息
	Theron::Receiver receiver;
	Theron::Catcher<ReadRequest> resultCatcher;
	receiver.RegisterHandler(&resultCatcher, &Theron::Catcher<ReadRequest>::Push);

	//创建一个dispatcher用来处理工作，一共MAX_FILES个workers
	Dispatcher<ReadRequest> dispatcher(framework, MAX_FILES);

	//发送工作请求，每个工作请求是命令行上输入的文件名
	for (int i = 0; i < MAX_FILES && i + 1 < argc; ++i)
	{
		unsigned char *const buffer = new unsigned char[MAX_FILE_SIZE];
		const ReadRequest message(
			receiver.GetAddress(),
			argv[i + 1],
			buffer,
			MAX_FILE_SIZE
		);
		framework.Send(message, receiver.GetAddress(), dispatcher.GetAddress());
	}

	//等待所有线程反馈
	for (int i = 0; i < argc; ++i)
	{
		receiver.Wait();
	}

	//处理结果，打印文件的字节大小
	ReadRequest result;
	Theron::Address from;
	while (!resultCatcher.Empty())
	{
		resultCatcher.Pop(result, from);
		printf("read %d bytes from file '%s'\n", result.mFileSize, result.mFileName);

		// 释放空间
		delete[] result.mBuffer;
	}
}