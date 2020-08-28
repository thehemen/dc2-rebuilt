#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <engine.h>

#ifndef HTTP_MANAGER_H
#define HTTP_MANAGER_H

using namespace std;

enum HTTPRequestType { Index, Remove, Rank };

struct HTTPRequest
{
	int index;
	HTTPRequestType type;
	map<string, string> str_val;
	map<string, int> int_val;

	HTTPRequest()
	{
		this->index = -1;
	}

	HTTPRequest(string filename, int seconds, string content)
	{
		type = HTTPRequestType::Index;
		str_val["filename"] = filename;
		int_val["seconds"] = seconds;
		str_val["content"] = content;
	}

	HTTPRequest(string filename)
	{
		type = HTTPRequestType::Remove;
		str_val["filename"] = filename;
	}

	HTTPRequest(int period, string lang_code, string category)
	{
		type = HTTPRequestType::Rank;
		int_val["period"] = period;
		str_val["lang_code"] = lang_code;
		str_val["category"] = category;
	}
};

struct HTTPResponse
{
	bool is_ready;
	int code;
	string body;

	HTTPResponse()
	{
		is_ready = false;
		code = 0;
	}

	HTTPResponse(int code)
	{
		is_ready = true;
		this->code = code;
	}

	HTTPResponse(int code, string body)
	{
		is_ready = true;
		this->code = code;
		this->body = body;
	}
};

class HTTPRequestQueue
{
	int index;
	queue<HTTPRequest> requests;
public:
	HTTPRequestQueue()
	{
		index = 0;
		requests = queue<HTTPRequest>();
	}

	int push(HTTPRequest &request)
	{
		request.index = index;
		requests.push(request);
		index++;
		return request.index;
	}

	HTTPRequest& front()
	{
		return requests.front();
	}

	void pop()
	{
		if(requests.size() > 0)
		{
			requests.pop();
		}
	}

	size_t size()
	{
		return requests.size();
	}
};

class HTTPResponseQueue
{
	map<int, HTTPResponse> responses;
public:
	HTTPResponseQueue()
	{
		responses = map<int, HTTPResponse>();
	}

	bool is_given(int index)
	{
		return responses.count(index);
	}

	HTTPResponse get(int index)
	{
		HTTPResponse response = responses[index];
		responses.erase(index);
		return response;
	}

	void set(int index, HTTPResponse &response)
	{
		responses[index] = response;
	}
};

class HTTPManager
{
	bool is_ready;

	int thread_num;
	Engine engine;

	vector<thread> threads;

	mutex in_mu;
	HTTPRequestQueue requests;

	mutex out_mu;
	HTTPResponseQueue responses;
public:
	HTTPManager(int thread_num, Engine& engine)
	{
		this->thread_num = thread_num;
		this->engine = engine;

		threads = vector<thread>();
		requests = HTTPRequestQueue();
		responses = HTTPResponseQueue();

		is_ready = false;
	}

	void poll()
	{
		thread init_t([&] (HTTPManager* httpManager) { httpManager->init(); }, this);

		for(int i = 0; i < thread_num; ++i)
		{
			thread t([&] (HTTPManager* httpManager) { httpManager->update(); }, this);
			threads.push_back(move(t));
		}

		init_t.join();

		for(int i = 0; i < thread_num; ++i)
		{
			threads[i].join();
		}

		while(true) {}
	}

	int index(string filename, int seconds, string content)
	{
		HTTPRequest req(filename, seconds, content);
		in_mu.lock();
		int req_index = requests.push(req);
		in_mu.unlock();
		return req_index;
	}

	int remove(string filename)
	{
		HTTPRequest req(filename);
		in_mu.lock();
		int req_index = requests.push(req);
		in_mu.unlock();
		return req_index;
	}

	int rank(int period, string lang_code, string category)
	{
		HTTPRequest req(period, lang_code, category);
		in_mu.lock();
		int req_index = requests.push(req);
		in_mu.unlock();
		return req_index;
	}

	HTTPResponse get_response(int index)
	{
		if(responses.is_given(index))
		{
			out_mu.lock();
			HTTPResponse res = responses.get(index);
			out_mu.unlock();
			return res;
		}
		else
		{
			return HTTPResponse();
		}
	}

private:
	void init()
	{
		engine.run_http_loading();
		is_ready = true;
	}

	void update()
	{
		while(true)
		{
			in_mu.lock();
			int request_num = requests.size();

			if(request_num > 0)
			{
				HTTPRequest req = requests.front();
				requests.pop();
				in_mu.unlock();

				HTTPResponse res;

				if(is_ready)
				{
					switch(req.type)
					{
						case HTTPRequestType::Index:
						{
							int status = engine.run_http_indexing(req.str_val["filename"],
								req.int_val["seconds"],
								req.str_val["content"]);
							res = HTTPResponse(status);
							break;
						}

						case HTTPRequestType::Remove:
						{
							int status = engine.run_http_removing(req.str_val["filename"]);
							res = HTTPResponse(status);
							break;
						}

						case HTTPRequestType::Rank:
						{
							auto status_body = engine.run_http_ranking(req.int_val["period"],
								req.str_val["lang_code"],
								req.str_val["category"]);
							res = HTTPResponse(get<0>(status_body), get<1>(status_body));
							break;
						}
					}
				}
				else
				{
					int status = 503;
					res = HTTPResponse(status);
				}

				out_mu.lock();
				responses.set(req.index, res);
				out_mu.unlock();
			}
			else
			{
				in_mu.unlock();
			}
		}
	}
};

#endif