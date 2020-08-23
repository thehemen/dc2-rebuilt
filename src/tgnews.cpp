#include <iostream>
#include <thread>
#include <engine.h>
#include <http_manager.h>
#include <http_server.h>

using namespace std;

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	if(argc != 3)
    {
        cout << "./tgnews languages source_dir" << endl;
        cout << "./tgnews news source_dir" << endl;
        cout << "./tgnews categories source_dir" << endl;
        cout << "./tgnews threads source_dir" << endl;
        cout << "./tgnews server port" << endl;
        return 1;
    }

    string op_type = argv[1];
    string op_value = argv[2];
    Engine engine("settings.json");

	if(op_type == "languages")
	{
		cout << engine.run_cli_languages(op_value) << endl;
	}
	else if(op_type == "news")
	{
		cout << engine.run_cli_news(op_value) << endl;
	}
	else if(op_type == "categories")
	{
		cout << engine.run_cli_categories(op_value) << endl;
	}
	else if(op_type == "threads")
	{
		cout << engine.run_cli_threads(op_value) << endl;
	}
    else if(op_type == "server")
    {
    	string address("localhost");
    	int port = stoi(op_value);
		int thread_num = engine.thread_num;
		int keep_alive_count = engine.keep_alive_count;

		HTTPManager httpManager(thread_num, engine);
		thread httpManThread([&] (HTTPManager* httpManager) { httpManager->poll(); }, &httpManager);
		HTTPServer httpServer(address, port, keep_alive_count);
		thread httpSvrThread([&] (HTTPServer* httpServer) { httpServer->run(httpManager); }, &httpServer);
		httpManThread.join();
		httpSvrThread.join();
	}
	return 0;
}