#include <vector>
#include <map>
#include <tuple>
#include <fstream>
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
	int thread_num;
	int keep_alive_count;

	Engine() {}

	Engine(string filename)
	{
		ifstream i(filename);
		json j;
		i >> j;

		indent_space_amount = j["indent_space_amount"];
		thread_num = j["server"]["thread_num"];
		keep_alive_count = j["server"]["keep_alive_count"];

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

			if(lang_code == "en")
			{
				articles[0].articles.push_back(filename);
			}
			else if(lang_code == "ru")
			{
				articles[1].articles.push_back(filename);
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
					articles["articles"].push_back(filename);
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
					string category = categoryClassifier.classify(article.get_text_tk(), lang_code);
					int category_index = get_index_by_string(categories, category);
					articles[category_index].articles.push_back(filename);
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

	int run_http_indexing(string filename, int seconds, string content)
	{
		int status = 201;
		return status;
	}

	int run_http_removing(string filename)
	{
		int status = 204;
		return status;
	}

	tuple<int, string> run_http_ranking(int period, string lang_code, string category)
	{
		map<string, vector<ra::RankedArticles>> articles;
		articles["threads"] = vector<ra::RankedArticles>();
		tuple<int, string> status_body(200, json(articles).dump(indent_space_amount));
		return status_body;
	}
};

#endif