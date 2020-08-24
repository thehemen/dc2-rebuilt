#include <vector>
#include <map>
#include <tuple>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <omp.h>

#include <article.h>
#include <utils.h>
#include <lang_detector.h>
#include <news_detector.h>
#include <category_classifier.h>
#include <thread_manager.h>

#ifndef ENGINE_H
#define ENGINE_H

using namespace std;
using json = nlohmann::json;

namespace la
{
    struct LanguageArticles {
        string lang_code;
        vector<string> articles;
    };

    void to_json(json& j, const LanguageArticles& p) {
        j = json{{"lang_code", p.lang_code}, {"articles", p.articles}};
    }

    void from_json(const json& j, LanguageArticles& p) {
        j.at("lang_code").get_to(p.lang_code);
        j.at("articles").get_to(p.articles);
    }
}

namespace ca {
    struct CategoryArticles {
        string category;
        vector<string> articles;
    };

    void to_json(json& j, const CategoryArticles& p) {
        j = json{{"category", p.category}, {"articles", p.articles}};
    }

    void from_json(const json& j, CategoryArticles& p) {
        j.at("category").get_to(p.category);
        j.at("articles").get_to(p.articles);
    }
}

namespace ta {
    struct ThreadArticles {
        string title;
        vector<string> articles;

        bool operator< (const ThreadArticles &other) const
        {
            return articles.size() > other.articles.size();
        }
    };

    void to_json(json& j, const ThreadArticles& p) {
        j = json{{"title", p.title}, {"articles", p.articles}};
    }

    void from_json(const json& j, ThreadArticles& p) {
        j.at("title").get_to(p.title);
        j.at("articles").get_to(p.articles);
    }
}

namespace ra {
    struct RankedArticles {
        string title;
        string category;
        vector<string> articles;

        bool operator< (const RankedArticles &other) const
        {
            return articles.size() > other.articles.size();
        }
    };

    void to_json(json& j, const RankedArticles& p) {
        j = json{{"title", p.title}, {"category", p.category}, {"articles", p.articles}};
    }

    void from_json(const json& j, RankedArticles& p) {
        j.at("title").get_to(p.title);
        j.at("category").get_to(p.category);
        j.at("articles").get_to(p.articles);
    }
}

class Engine
{
    LanguageDetector languageDetector;
    NewsDetector newsDetector;
    CategoryClassifier categoryClassifier;
    ThreadManager threadManager;

public:
	int indent_space_amount;

	string address;
	int thread_num;
	int keep_alive_count;
	string index_dir;

	Engine() {}

	Engine(string filename)
	{
		ifstream i(filename);
		json j;
		i >> j;

		indent_space_amount = j["indent_space_amount"];

		address = j["server"]["address"];
		thread_num = j["server"]["thread_num"];
		keep_alive_count = j["server"]["keep_alive_count"];
		index_dir = j["server"]["index_dir"];

		languageDetector = LanguageDetector(j["languages"]["token_share"],
			j["languages"]["en_common_share"]);
		newsDetector = NewsDetector();
		categoryClassifier = CategoryClassifier(j["categories"]["filename"]["en"],
			j["categories"]["filename"]["ru"],
			j["categories"]["min_char_share"],
			j["categories"]["min_token_count"]);
		threadManager = ThreadManager(j["threads"]["min_similar_token_count"],
			j["threads"]["min_similarity"]);

		omp_set_num_threads(thread_num);
	}

	string run_cli_languages(string source_dir)
	{
		vector<la::LanguageArticles> articles;
		articles.push_back(la::LanguageArticles{"en", vector<string>()});
		articles.push_back(la::LanguageArticles{"ru", vector<string>()});
		vector<string> paths = get_filename_list(source_dir);

		#pragma omp parallel for
		for (auto it = paths.begin(); it < paths.end(); it++)
		{
			string path(*it);
	    	string filename = get_filename_only(path);
			Article article(path.c_str(), filename);
			string lang_code = languageDetector.detect(article.get_text_tk());

			#pragma omp critical
			{
				if(lang_code == "en")
				{
					articles[0].articles.push_back(filename);
				}
				else if(lang_code == "ru")
				{
					articles[1].articles.push_back(filename);
				}
			}
	    }

		return json(articles).dump(indent_space_amount);
	}

	string run_cli_news(string source_dir)
	{
		map<string, vector<string>> articles;
		articles["articles"] = vector<string>();
		vector<string> paths = get_filename_list(source_dir);

		#pragma omp parallel for
		for (auto it = paths.begin(); it < paths.end(); it++)
		{
			string path(*it);
            string filename = get_filename_only(path);
			Article article(path.c_str(), filename);
			string lang_code = languageDetector.detect(article.get_text_tk());

			if(lang_code == "en" || lang_code == "ru")
			{
				if(newsDetector.is_news(article.get_header_tk(), lang_code))
				{
					#pragma omp critical
					{
						articles["articles"].push_back(filename);
					}
				}
			}
	    }

		return json(articles).dump(indent_space_amount);
	}

	string run_cli_categories(string source_dir)
	{
		vector<string> categories = { "society", "economy", "technology", "sports", "entertainment", "science", "other" };
		vector<ca::CategoryArticles> articles;

		for(const auto & category : categories)
		{
			articles.push_back(ca::CategoryArticles{category, vector<string>()});
		}

		vector<string> paths = get_filename_list(source_dir);

		#pragma omp parallel for
		for (auto it = paths.begin(); it < paths.end(); it++)
		{
			string path(*it);
            string filename = get_filename_only(path);
			Article article(path.c_str(), filename);
			string lang_code = languageDetector.detect(article.get_text_tk());

			if(lang_code == "en" || lang_code == "ru")
			{
				if(newsDetector.is_news(article.get_header_tk(), lang_code))
				{
					article.set_lang_code(lang_code);
					string category = categoryClassifier.classify(article.get_text_tk(), lang_code);
					int category_index = get_index_by_string(categories, category);

					#pragma omp critical
					{
						articles[category_index].articles.push_back(filename);
					}
				}
			}
	    }

		return json(articles).dump(indent_space_amount);
	}

	string run_cli_threads(string source_dir)
	{
		vector<ta::ThreadArticles> articles;
		vector<string> paths = get_filename_list(source_dir);

		#pragma omp parallel for
		for (auto it = paths.begin(); it < paths.end(); it++)
		{
			string path(*it);
            string filename = get_filename_only(path);
			Article article(path.c_str(), filename);
			string lang_code = languageDetector.detect(article.get_text_tk());

			if(lang_code == "en" || lang_code == "ru")
			{
				if(newsDetector.is_news(article.get_header_tk(), lang_code))
				{
					article.set_lang_code(lang_code);
					#pragma omp critical
					{
						threadManager.add(article);
					}
				}
			}
	    }

	    for(auto & [thread_id, news_thread] : threadManager.get_threads())
	    {
			articles.push_back(ta::ThreadArticles{threadManager.get_thread_title(thread_id), 
				news_thread.get_article_keys()});
	    }

	    sort(articles.begin(), articles.end());
		return json(articles).dump(indent_space_amount);
	}

	int run_http_loading()
	{
		vector<string> paths = get_filename_list(index_dir);

		#pragma omp parallel for
		for (auto it = paths.begin(); it < paths.end(); it++)
		{
			string path(*it);
            string filename = get_filename_only(path);
			Article article(path.c_str(), filename);
			string lang_code = languageDetector.detect(article.get_text_tk());

			if(lang_code == "en" || lang_code == "ru")
			{
				if(newsDetector.is_news(article.get_header_tk(), lang_code))
				{
					string category = categoryClassifier.classify(article.get_text_tk(), lang_code);
					article.set_category(category);
					article.set_lang_code(lang_code);

					#pragma omp critical
					{
						threadManager.add(article);
					}
				}
			}
	    }

	    return 1;
	}

	int run_http_indexing(string path, int seconds, string content)
	{
		int status = 204;  // status by default
		bool is_saved_to_file = false;
		bool is_already_indexed = false;
		string filename = get_filename_only(path);
		string full_path = get_full_path(index_dir, filename);
		Article article(utf8_to_wstring(content), filename);

		for(const auto & [article_key, article] : threadManager.get_articles())
		{
			if(article_key == filename)
			{
				is_already_indexed = true;
				break;
			}
		}

		time_t last_article_time = threadManager.get_all_last_published_time();
		time_t published_time = article.get_published_time();
		int diff_time = difftime(published_time, last_article_time);
		bool is_ttl_exceeded = diff_time > seconds && last_article_time > 0;

		if(is_already_indexed)
		{
			if(!is_ttl_exceeded)
			{
				is_saved_to_file = true;
				threadManager.update(article);
			}
			else
			{
				threadManager.remove(filename);
				remove_file(full_path.c_str());
			}
		}
		else
		{
			if(!is_ttl_exceeded)
			{
				string lang_code = languageDetector.detect(article.get_text_tk());

				if(lang_code == "en" || lang_code == "ru")
				{
					if(newsDetector.is_news(article.get_header_tk(), lang_code))
					{
						article.set_lang_code(lang_code);
						string category = categoryClassifier.classify(article.get_text_tk(), lang_code);
						article.set_category(category);
						threadManager.add(article);
						is_saved_to_file = true;
						status = 201;
					}
				}
			}
		}

		if(is_saved_to_file)
		{
			save_file(full_path.c_str(), content);
		}

		return status;
	}

	int run_http_removing(string path)
	{
		int status;
		bool is_already_indexed = false;
		string filename = get_filename_only(path);
		string full_path = get_full_path(index_dir, filename);

		for(const auto & [article_key, article] : threadManager.get_articles())
		{
			if(article_key == filename)
			{
				is_already_indexed = true;
				break;
			}
		}

		if(is_already_indexed)
		{
			threadManager.remove(filename);
			remove_file(full_path.c_str());
			status = 204;
		}
		else
		{
			status = 404;
		}

		return status;
	}

	tuple<int, string> run_http_ranking(int period, string lang_code, string category)
	{
		int status = 200;
		map<string, vector<ra::RankedArticles>> articles;
		articles["threads"] = vector<ra::RankedArticles>();
		time_t time_now = get_time_now();

		for(auto [thread_index, articles_thread] : threadManager.get_threads())
		{
			time_t last_published_time = threadManager.get_last_published_time(thread_index);
			int diff_time = difftime(time_now, last_published_time);

			if(diff_time > period)
			{
				continue;
			}

			string lang_code_now = threadManager.get_thread_lang_code(thread_index);

			if(lang_code != lang_code_now)
			{
				continue;
			}

			string category_now = threadManager.get_thread_category(thread_index);

			if(category != "any" && category != category_now)
			{
				continue;
			}

			string title = threadManager.get_thread_title(thread_index);
			vector<string> article_keys = articles_thread.get_article_keys();
			articles["threads"].push_back(ra::RankedArticles{ title, category_now, article_keys});
		}

		sort(articles["threads"].begin(), articles["threads"].end());
		return tuple<int, string>(status, json(articles).dump(indent_space_amount));
	}
};

#endif