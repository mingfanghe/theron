#include <string.h>
#include <iostream>
#include <Theron/Theron.h>
using namespace std;

// ��ɴ�ӡ�ַ�����Actor,��actor�յ���Ϣ�Զ�������Ϣ����������ӡ��Ϣ����
// �̳���Theron::Actor. 
class Printer : public Theron::Actor
{
public:
	// �ṹ��, ����framework������ 
	Printer(Theron::Framework &framework) : Theron::Actor(framework)
	{
		// ע����Ϣ������  ��Ϣ������ΪPrint()����������ͨ��Printer���캯����ע���
		RegisterHandler(this, &Printer::Print);
	}
private:
	// std::string���͵���Ϣ������ 
	void Print(const std::string &message, const Theron::Address from)
	{
		// ��ӡ�ַ��� 
		printf("%s\n", message.c_str());
		// ����һ����٣�dummy����Ϣ��ʵ��ͬ�� 
		Send(0, from);
	}
};


int main()
{
	// ����һ��framework���󣬲���ʵ����һ��Printer��actor��������. 
	Theron::Framework framework;
	Printer printer(framework);
	// ����һ��receiver����������actor�ķ�����Ϣ 
	Theron::Receiver receiver;
	// ����һ����Ϣ��Printer. 
	// ���Ǵ���receiver�ĵ�ַ����'from'�ĵ�ַ 
	if (!framework.Send(
		std::string("Hello world!"),
		receiver.GetAddress(),
		printer.GetAddress()))
	{
		printf("ERROR: Failed to send message\n");
	}
	// ʹ�������Ϣ��ʵ��ͬ����ȷ�������߳��������
	receiver.Wait();//�����ȴ����յ�actor�ķ�����Ϣ�Ž������̣߳�Ҳ�����������̡�
	system("pause");
}