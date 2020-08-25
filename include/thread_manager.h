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
	map<string, Article> articles;
public:
	Thread() {}

	Thread(string article_key, Article article)
	{
		articles = map<string, Article>();
		articles[article_key] = article;
	}

	void add(string article_key, Article article)
	{
		articles[article_key] = article;
	}

	void update(string article_key, Article article)
	{
		articles[article_key] = article;
	}

	bool remove(string article_key)
	{
		return remove_map_elem_if_exists(articles, article_key);
	}

	vector<string> get_article_keys()
	{
		vector<string> article_keys;

		for(const auto & [article_key, article] : articles)
		{
			article_keys.push_back(article_key);
		}

		return article_keys;
	}

	Article get_article_by_key(string article_key)
	{
		if(articles.count(article_key) != 0)
		{
			return articles[article_key];
		}
		else
		{
			return Article();
		}
	}

	string get_lang_code()
	{
		string lang_code("other");

		if(articles.size() > 0)
		{
			lang_code = articles.begin()->second.get_lang_code();
		}

		return lang_code;
	}

	time_t get_last_published_time()
	{
		time_t last_published_time = 0;

		for(auto & [article_key, article] : articles)
		{
			time_t published_time = article.get_published_time();

			if(published_time > last_published_time)
			{
				last_published_time = published_time;
			}
		}

		return last_published_time;
	}

	string get_title()
	{
		time_t last_published_time = 0;
		wstring last_article_title(L"");

		for(auto & [article_key, article] : articles)
		{
			time_t published_time = article.get_published_time();

			if(published_time > last_published_time)
			{
				last_published_time = published_time;
				last_article_title = article.get_header_raw();
			}
		}

		return wstring_to_utf8(last_article_title);
	}

	string get_category()
	{
		map<string, int> category_counter;
		string category("other");

		for(auto & [article_key, article] : articles)
		{
			string category_now = article.get_category();

			if(category_counter.count(category_now) == 0)
			{
				category_counter[category_now] = 0;
			}

			category_counter[category_now]++;
		}

		auto category_with_count = get_pair_by_max_value<string>(category_counter);
		category = get<0>(category_with_count);
		return category;
	}
};

class ThreadManager
{
	double min_similarity;

	map<int, Thread> threads;
	map<string, int> thread_id_by_key;
	map<wstring, vector<string>> keys_by_token;

	int last_thread_id;
	time_t last_article_time;
public:
	ThreadManager() {}

	ThreadManager(double min_similarity)
	{
		this->min_similarity = min_similarity;

		threads = map<int, Thread>();
		thread_id_by_key = map<string, int>();
		keys_by_token = map<wstring, vector<string>>();

		last_thread_id = 0;
		last_article_time = 0;
	}

	bool add(Article& article)
	{
		string article_key = article.get_filename_only();

		if(thread_id_by_key.count(article_key) == 0)
		{
			int thread_id = get_best_thread_id(article);

			if(thread_id == -1)
			{
				thread_id = last_thread_id;
				threads[thread_id] = Thread(article_key, article);
				last_thread_id++;
			}
			else
			{
				threads[thread_id].add(article_key, article);
			}

			thread_id_by_key[article_key] = thread_id;

			for(const auto & token : article.get_header_tk())
			{
				if(keys_by_token.count(token) == 0)
				{
					keys_by_token[token] = vector<string>();
				}

				keys_by_token[token].push_back(article_key);
			}

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
		if(thread_id_by_key.count(article_key) != 0)
		{
			int thread_id = thread_id_by_key[article_key];

			if(threads.count(thread_id) != 0)
			{
				threads[thread_id].remove(article_key);
				bool success_rmv_key = remove_map_elem_if_exists(thread_id_by_key, article_key);

				if(success_rmv_key)
				{
					bool success_rmv_thr = remove_map_elem_if_exists(threads, thread_id);
					return success_rmv_thr;
				}
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

		if(thread_id_by_key.count(article_key) != 0)
		{
			bool success_rmv = remove(article_key);

			if(success_rmv)
			{
				bool success_add = add(article);
				return success_add;
			}
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

	bool is_article_available_by_key(string article_key)
	{
		return thread_id_by_key.count(article_key) != 0;
	}

	map<int, Thread> get_index_threads()
	{
		return threads;
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
					if(thread_id_by_key.count(article_key) == 0)
					{
						continue;
					}

					int thread_id_now = thread_id_by_key[article_key];

					if(threads[thread_id_now].get_lang_code() != lang_code)
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

		if(article_counter.size() > 0)
		{
			auto article_with_count = get_pair_by_max_value<string>(article_counter);
			string article_key = get<0>(article_with_count);
			int similar_token_count = get<1>(article_with_count);

			int thread_id_best = thread_id_by_key[article_key];
			Article other = threads[thread_id_best].get_article_by_key(article_key);

			int first_token_count = tokens.size();
			int second_token_count = other.get_header_tk().size();
			int all_token_count = first_token_count + second_token_count - similar_token_count;
			double similarity = (double)similar_token_count / all_token_count;

			if(similarity > min_similarity)
			{
				thread_id = thread_id_best;
			}
		}

		return thread_id;
	}
};

#endif