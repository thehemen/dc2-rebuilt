#include <vector>
#include <map>
#include <algorithm>
#include <article.h>
#include <utils.h>

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

using namespace std;

class Thread
{
	string lang_code;
	vector<string> article_keys;
public:
	Thread() {}

	Thread(Article& article)
	{
		article_keys = vector<string>{ article.get_filename_only() };
		lang_code = article.get_lang_code();
	}

	void add(Article& article)
	{
		article_keys.push_back(article.get_filename_only());
	}

	void remove(string article_key)
	{
		remove_if_exists(article_keys, article_key);
	}

	vector<string> get_article_keys()
	{
		return article_keys;
	}

	string get_lang_code()
	{
		return lang_code;
	}
};

class ThreadManager
{
	double min_similarity;

	map<int, Thread> threads;
	map<string, Article> articles;
	map<wstring, vector<string>> keys_by_token;

	int last_thread_id;
	time_t last_article_time;
public:
	ThreadManager() {}

	ThreadManager(double min_similarity)
	{
		this->min_similarity = min_similarity;

		threads = map<int, Thread>();
		articles = map<string, Article>();
		keys_by_token = map<wstring, vector<string>>();

		last_thread_id = 0;
		last_article_time = 0;
	}

	bool add(Article& article)
	{
		string article_key = article.get_filename_only();

		if(articles.count(article_key) == 0)
		{
			int thread_id = get_best_thread_id(article);

			if(thread_id == -1)
			{
				thread_id = last_thread_id;
				threads[thread_id] = Thread(article);
				last_thread_id++;
			}
			else
			{
				threads[thread_id].add(article);
			}

			for(const auto & token : article.get_header_tk())
			{
				if(keys_by_token.count(token) == 0)
				{
					keys_by_token[token] = vector<string>();
				}

				keys_by_token[token].push_back(article_key);
			}

			article.set_thread_id(thread_id);
			articles[article_key] = article;

			time_t published_time = article.get_published_time();

			if(published_time > last_article_time)
			{
				last_article_time = published_time;
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	bool remove(string article_key)
	{
		if(articles.count(article_key) != 0)
		{
			int thread_id = articles[article_key].get_thread_id();

			if(threads.count(thread_id) != 0)
			{
				articles.erase(articles.find(article_key));
				threads[thread_id].remove(article_key);

				if(threads[thread_id].get_article_keys().size() == 0)
				{
					threads.erase(threads.find(thread_id));
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	bool update(Article& article)
	{
		string article_key = article.get_filename_only();

		if(articles.count(article_key) != 0)
		{
			remove(article_key);
			add(article);
			return true;
		}
		else
		{
			return false;
		}
	}

	time_t get_all_last_published_time()
	{
		return last_article_time;
	}

	time_t get_last_published_time(int thread_id)
	{
		time_t last_published_time = 0;

		if(threads.count(thread_id) != 0)
		{
			for(auto article_key : threads[thread_id].get_article_keys())
			{
				time_t published_time = articles[article_key].get_published_time();

				if(published_time > last_published_time)
				{
					last_published_time = published_time;
				}
			}
		}

		return last_published_time;
	}

	string get_thread_title(int thread_id)
	{
		time_t last_published_time = 0;
		wstring last_article_title(L"");

		if(threads.count(thread_id) != 0)
		{
			for(auto article_key : threads[thread_id].get_article_keys())
			{
				time_t published_time = articles[article_key].get_published_time();

				if(published_time > last_published_time)
				{
					last_published_time = published_time;
					last_article_title = articles[article_key].get_header_raw();
				}
			}
		}

		return wstring_to_utf8(last_article_title);
	}

	string get_thread_category(int thread_id)
	{
		map<string, int> category_counter;
		string category("other");

		if(threads.count(thread_id) != 0)
		{
			for(auto article_key : threads[thread_id].get_article_keys())
			{
				string category_now = articles[article_key].get_category();

				if(category_counter.count(category_now) == 0)
				{
					category_counter[category_now] = 0;
				}

				category_counter[category_now]++;
			}

			auto category_with_count = get_pair_by_max_value<string>(category_counter);
			category = get<0>(category_with_count);
		}

		return category;
	}

	string get_thread_lang_code(int thread_id)
	{
		string lang_code("other");

		if(threads.count(thread_id) != 0)
		{
			lang_code = threads[thread_id].get_lang_code();
		}

		return lang_code;
	}

	map<int, Thread> get_threads()
	{
		return threads;
	}

	map<string, Article> get_articles()
	{
		return articles;
	}

private:
	int get_best_thread_id(Article& article)
	{
		int thread_id = -1;
		map<string, int> article_counter;
		vector<wstring> tokens = article.get_header_tk();
		string lang_code = article.get_lang_code();

		for(const auto & token : tokens)
		{
			if(keys_by_token.count(token) != 0)
			{
				for(const auto & article_key : keys_by_token[token])
				{
					if(articles[article_key].get_lang_code() != lang_code)
					{
						continue;
					}

					if(article_counter.count(article_key) == 0)
					{
						article_counter[article_key] = 0;
					}

					article_counter[article_key]++;
				}
			}
		}

		auto article_with_count = get_pair_by_max_value<string>(article_counter);
		string article_key = get<0>(article_with_count);
		int similar_token_count = get<1>(article_with_count);

		int first_token_count = tokens.size();
		int second_token_count = articles[article_key].get_header_tk().size();
		int all_token_count = first_token_count + second_token_count - similar_token_count;
		double similarity = (double)similar_token_count / all_token_count;

		if(similarity > min_similarity)
		{
			thread_id = articles[article_key].get_thread_id();
		}

		return thread_id;
	}
};

#endif