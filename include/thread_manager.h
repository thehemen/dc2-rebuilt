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
	int token_num;
public:
	Thread() {}

	Thread(Article& article)
	{
		article_keys = vector<string>{ article.get_filename_only() };
		lang_code = article.get_lang_code();
		token_num = article.get_upper_tokens().size();
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

	int get_token_num()
	{
		return token_num;
	}
};

class ThreadManager
{
	map<string, int> min_similar_token_count;
	map<string, double> threads_min_similarity;

	int last_thread_id;
	map<int, Thread> threads;
	map<string, Article> articles;
	map<wstring, vector<int>> ids_by_token;
	time_t last_article_time;
public:
	ThreadManager() {}

	ThreadManager(map<string, int> min_similar_token_count, map<string, double> threads_min_similarity)
	{
		this->min_similar_token_count = min_similar_token_count;
		this->threads_min_similarity = threads_min_similarity;

		last_thread_id = 0;
		threads = map<int, Thread>();
		articles = map<string, Article>();
		ids_by_token = map<wstring, vector<int>>();
		last_article_time = 0;
	}

	bool add(Article& article)
	{
		string article_key = article.get_filename_only();
		string lang_code = article.get_lang_code();

		if(articles.count(article_key) == 0)
		{
			vector<wstring> uppercase_tokens = article.get_upper_tokens();
			int thread_id = get_best_thread_id(uppercase_tokens, lang_code, article_key);

			if(thread_id == -1)
			{
				thread_id = last_thread_id;
				threads[thread_id] = Thread(article);
				thread_id = last_thread_id;
				last_thread_id++;

				for(const auto & uppercase_token : uppercase_tokens)
				{
					if(ids_by_token.count(uppercase_token) == 0)
					{
						ids_by_token[uppercase_token] = vector<int>();
					}

					ids_by_token[uppercase_token].push_back(thread_id);
				}
			}
			else
			{
				threads[thread_id].add(article);
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

			auto category_with_count = get_pair_by_max_value(category_counter);
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
	int get_best_thread_id(vector<wstring> uppercase_tokens, string lang_code, string article_key)
	{
		map<int, int> thread_counter;

		for(const auto & uppercase_token : uppercase_tokens)
		{
			if(ids_by_token.count(uppercase_token) != 0)
			{
				for(const auto & thread_id : ids_by_token[uppercase_token])
				{
					string other_lang_code = threads[thread_id].get_lang_code();

					if(other_lang_code != lang_code)
					{
						continue;
					}

					if(thread_counter.count(thread_id) == 0)
					{
						thread_counter[thread_id] = 0;
					}

					thread_counter[thread_id]++;
				}
			}
		}

		int article_token_num = uppercase_tokens.size();
		double max_similarity = 0.0;
		int best_thread_id = -1;

		for(const auto & [thread_id, similar_token_num] : thread_counter)
		{
			if(similar_token_num >= min_similar_token_count[lang_code])
			{
				int thread_token_num = threads[thread_id].get_token_num();
				int all_token_num = article_token_num + thread_token_num - similar_token_num;
				double similarity = (double)similar_token_num / all_token_num;

				if(similarity > threads_min_similarity[lang_code])
				{
					if(similarity > max_similarity)
					{
						max_similarity = similarity;
						best_thread_id = thread_id;
					}
				}
			}
		}

		return best_thread_id;
	}
};

#endif