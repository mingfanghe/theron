#include <Theron/Theron.h> 

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

// ���ݶ�ȡ����: ��ȡһ�������ļ������ݵ�������.
struct ReadRequest
{
public:

	explicit ReadRequest(
		const Theron::Address client = Theron::Address(),
		const char *const fileName = 0,
		unsigned char *const buffer = 0,
		const unsigned int bufferSize = 0) :
		mClient(client),
		mFileName(fileName),
		mProcessed(false),
		mBuffer(buffer),
		mBufferSize(bufferSize),
		mFileSize(0)
	{
	}
	void Process()
	{
		mProcessed = true;
		mFileSize = 0;
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

static const int MAX_FILES = 16;
static const int MAX_FILE_SIZE = 16384;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Expected up to 16 file name arguments.\n");
	}
	// ����һ��workerȥ������ 
	Theron::Framework framework;
	Worker<ReadRequest> worker(framework);
	// ע��һ��receiver������catcher���񷵻صĽ��
	Theron::Receiver receiver;
	Theron::Catcher<ReadRequest> resultCatcher;
	receiver.RegisterHandler(&resultCatcher, &Theron::Catcher<ReadRequest>::Push);
	// ��������ÿ���ļ�������Ϊ������Ϣ
	for (int i = 0; i < MAX_FILES && i + 1 < argc; ++i)
	{
		unsigned char *const buffer = new unsigned char[MAX_FILE_SIZE];
		const ReadRequest message(
			receiver.GetAddress(),
			argv[i + 1],
			buffer,
			MAX_FILE_SIZE);
		framework.Send(message, receiver.GetAddress(), worker.GetAddress());
	}
	// �ȴ����н�� 
	for (int i = 1; i < argc; ++i)
	{
		receiver.Wait();
	}
	// �����������ǽ���ӡ�ļ�������
	ReadRequest result;
	Theron::Address from;
	while (!resultCatcher.Empty())
	{
		resultCatcher.Pop(result, from);
		printf("Read %d bytes from file '%s'\n", result.mFileSize, result.mFileName);
		// �ͷ�����Ŀռ� 
		delete[] result.mBuffer;
	}
	system("pause");
}