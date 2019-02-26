#include <string.h>
#include <iostream>
#include <Theron/Theron.h>
using namespace std;

// 完成打印字符串的Actor,当actor收到消息自动调动消息处理函数来打印消息内容
// 继承自Theron::Actor. 
class Printer : public Theron::Actor
{
public:
	// 结构体, 传递framework给基类 
	Printer(Theron::Framework &framework) : Theron::Actor(framework)
	{
		// 注册消息处理函数  消息处理函数为Print()。处理函数是通过Printer构造函数来注册的
		RegisterHandler(this, &Printer::Print);
	}
private:
	// std::string类型的消息处理函数 
	void Print(const std::string &message, const Theron::Address from)
	{
		// 打印字符串 
		printf("%s\n", message.c_str());
		// 返回一个虚假（dummy）消息来实现同步 
		Send(0, from);
	}
};


int main()
{
	// 构造一个framework对象，并且实例化一个Printer的actor由它管理. 
	Theron::Framework framework;
	Printer printer(framework);
	// 构造一个receiver对象来接收actor的反馈消息 
	Theron::Receiver receiver;
	// 发送一条消息给Printer. 
	// 我们传递receiver的地址当做'from'的地址 
	if (!framework.Send(
		std::string("Hello world!"),
		receiver.GetAddress(),
		printer.GetAddress()))
	{
		printf("ERROR: Failed to send message\n");
	}
	// 使用虚假消息来实现同步，确保所有线程完成任务
	receiver.Wait();//用来等待接收到actor的反馈消息才结束主线程，也就是整个进程。
	system("pause");
}