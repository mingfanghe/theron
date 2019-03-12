#include <vector>
#include <queue>
#include <Theron/Theron.h>

using namespace std;

template <class WorkMessage>
class Worker : public Theron::Actor
{
public:
	// ���캯�� 
	Worker(Theron::Framework &framework) : Theron::Actor(framework)
	{
		RegisterHandler(this, &Worker::Handler);
	}
private:
	// ��Ϣ������ 
	void Handler(const WorkMessage &message, const Theron::Address from)

	{
		// ��Ϣ������const������������Ҫ���������ı��������� 
		WorkMessage result(message);
		result.Process();
		// �����߳���Ϣ��������
		Send(result, from);
	}
};

//���ݶ�ȡ���󣺶�ȡһ�������ļ������ݵ�������
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
		// ���Դ��ļ� 
		FILE *const handle = fopen(mFileName, "rb");
		if (handle != 0)
		{
			// ��ȡ�����ļ�������ʵ�ʶ�ȡ���� 
			mFileSize = (uint32_t)fread(
				mBuffer,
				sizeof(unsigned char),
				mBufferSize,
				handle);

			fclose(handle);
		}
	}

	Theron::Address mClient;            // ����ͻ��ĵ�ַ
	const char *mFileName;              // �����ļ������� 
	bool mProcessed;                    // �ļ��Ƿ񱻶�ȡ��
	unsigned char *mBuffer;             // �ļ����ݵĻ�����
	unsigned int mBufferSize;           // �������Ĵ�С
	unsigned int mFileSize;             // �ļ����ֽڳ��� 
};

// һ����ǲactor����������
// ��ǲactor�����ڲ������Ϳ���һϵ��workers. 
// ��Э��workers���д���������
template <class WorkMessage>
class Dispatcher : public Theron::Actor
{
public:
	Dispatcher(Theron::Framework &framework, const int workerCount) :Theron::Actor(framework)
	{
		// ����workers���ҽ����Ǽ�����е�����
		for (int i = 0; i < workerCount; ++i)
		{
			//WorkersȺ����ʹ��new������ڹ������б���̬�����ģ�������һ��ָ�������У�������������������ʹ��delete����������١�
			mWorkers.push_back(new WorkerType(framework));
			mFreeQueue.push(mWorkers.back()->GetAddress());
		}
		RegisterHandler(this, &Dispatcher::Handler);
	}

	~Dispatcher()
	{
		//�ͷ�workers
		const int workerCount(static_cast<int>(mWorkers.size()));
		for (int i = 0; i < workerCount; ++i)
		{
			delete mWorkers[i];
		}
	}

private:
	typedef Worker<WorkMessage> WorkerType;

	// �ӿͻ��˴����ȡ����
	void Handler(const WorkMessage &message, const Theron::Address from)
	{
		// �Ƿ���������������������
		if (message.mProcessed)
		{
			// ���ͽ�����ظ�����ĵ����� 
			Send(message, message.mClient);

			// ��worker���뵽���е�����
			mFreeQueue.push(from);
		}
		else
		{
			// ��û�д���Ĺ����������빤������
			mWorkQueue.push(message);
		}

		// �����ڹ�������
		while (!mWorkQueue.empty() && !mFreeQueue.empty())
		{
			Send(mWorkQueue.front(), mFreeQueue.front());
			mFreeQueue.pop();
			mWorkQueue.pop();
		}
	}

	std::vector<WorkerType *> mWorkers; // ӵ�е�workers��ָ��
	std::queue<Theron::Address> mFreeQueue; // ����workers����.
	std::queue<WorkMessage> mWorkQueue; // δ��������Ϣ����
};

static const int MAX_FILES = 16;
static const int MAX_FILE_SIZE = 16384;

int main(int argc, char *argv[])
{
	//Theron::Framework������Ա��вι���
	//����һ��ӵ��n���̵߳�framework��nΪ�ļ���������Ŀ�ģ����Բ��ж�ȡ���е��ļ�
	//���ƹ����߳�Ϊǰ������������
	Theron::Framework::Parameters frameworkParams;
	frameworkParams.mThreadCount = MAX_FILES;
	frameworkParams.mProcessorMask = (1UL << 0) | (1UL << 1);
	Theron::Framework framework(frameworkParams);

	if (argc < 2)
	{
		printf("Excepted up to 16 file name arguments.\n");
	}

	//ע��һ��receive��һ����������������������Ϣ
	Theron::Receiver receiver;
	Theron::Catcher<ReadRequest> resultCatcher;
	receiver.RegisterHandler(&resultCatcher, &Theron::Catcher<ReadRequest>::Push);

	//����һ��dispatcher������������һ��MAX_FILES��workers
	Dispatcher<ReadRequest> dispatcher(framework, MAX_FILES);

	//���͹�������ÿ��������������������������ļ���
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

	//�ȴ������̷߳���
	for (int i = 0; i < argc; ++i)
	{
		receiver.Wait();
	}

	//����������ӡ�ļ����ֽڴ�С
	ReadRequest result;
	Theron::Address from;
	while (!resultCatcher.Empty())
	{
		resultCatcher.Pop(result, from);
		printf("read %d bytes from file '%s'\n", result.mFileSize, result.mFileName);

		// �ͷſռ�
		delete[] result.mBuffer;
	}
}