#include <string>
#include <http_manager.h>
#include <httplib.h>

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

using namespace std;
using namespace httplib;

class HTTPServer
{
	int keep_alive_count;
	string address;
	int port;
public:
	HTTPServer(string address, int port, int keep_alive_count)
	{
		this->address = address;
		this->port = port;
		this->keep_alive_count = keep_alive_count;
	}

	void run(HTTPManager& httpManager)
	{
		Server svr;
		svr.set_keep_alive_max_count(keep_alive_count);

		svr.Put(R"(/(\S+).html)", [&](const Request& req, Response& res)
        {
        	string filename = req.path.substr(1, req.path.size());
        	string content = req.body;
        	int seconds = 0;

            if (req.has_header("Cache-Control"))
            {
                string max_age = req.get_header_value("Cache-Control");
                int first_digit_pos = max_age.find_first_of("=") + 1;
                seconds = stoi(max_age.substr(first_digit_pos, max_age.size() - first_digit_pos + 1));
            }

            int res_index = httpManager.index(filename, seconds, content);
            HTTPResponse response;

			while(response.code == 0)
			{
				response = httpManager.get_response(res_index);
			}

			res.status = response.code;
        });

        svr.Delete(R"(/(\S+).html)", [&](const Request& req, Response& res)
        {
        	string filename = req.path.substr(1, req.path.size());
        	int res_index = httpManager.remove(filename);
        	HTTPResponse response;

			while(response.code == 0)
			{
				response = httpManager.get_response(res_index);
			}

			res.status = response.code;
        });

        svr.Get("/threads", [&](const Request& req, Response& res)
        {
        	map<string, string> params = {{"period", "0"}, {"lang_code", ""}, {"category", ""}};

            for(const auto & [key, val]: params)
            {
                const char *ckey = key.c_str();

                if (req.has_param(ckey))
                {
                    params[key] = req.get_param_value(ckey);
                }
            }

            int period = stoi(params["period"]);
            string lang_code = params["lang_code"];
            string category = params["category"];

            int res_index = httpManager.rank(period, lang_code, category);
            HTTPResponse response;

			while(response.code == 0)
			{
				response = httpManager.get_response(res_index);
			}

			res.status = response.code;
			res.set_content(response.body, "application/json");
        });

		svr.listen(address.c_str(), port);
	}
};

#endif