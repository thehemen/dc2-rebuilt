#include <vector>
#include <map>
#include <algorithm>
#include <article.h>
#include <utils.h>
#include <nlp_utils.h>

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

using namespace std;

class Thread
{
	vector<string> article_keys;
public:
	Thread() {}

	Thread(Article& article)
	{
		article_keys = vector<string>{ article.get_filename_only() };
	}

	void add(Article& article)
	{
		article_keys.push_back(article.get_filename_only());
	}

	void remove(string article_key)
	{
		auto position = find(article_keys.begin(), article_keys.end(), article_key);

		if (position != article_keys.end())
		{
		    article_keys.erase(position);
		}
	}

	vector<string> get_article_keys()
	{
		return article_keys;
	}
};

class ThreadManager
{
	map<string, int> min_similar_token_count;
	map<string, double> threads_min_similarity;

	int last_thread_index;
	map<int, Thread> threads;
	map<string, Article> articles;
	map<wstring, vector<string>> keys_by_token;
	time_t last_article_time;
public:
	ThreadManager() {}

	ThreadManager(map<string, int> min_similar_token_count, map<string, double> threads_min_similarity)
	{
		this->min_similar_token_count = min_similar_token_count;
		this->threads_min_similarity = threads_min_similarity;

		last_thread_index = 0;
		threads = map<int, Thread>();
		articles = map<string, Article>();
		keys_by_token = map<wstring, vector<string>>();
		last_article_time = 0;
	}

	bool add(Article& article)
	{
		string article_key = article.get_filename_only();
		string lang_code = article.get_lang_code();

		if(articles.count(article_key) == 0)
		{
			int thread_index = get_most_similar_thread_index(article, lang_code, article_key);

			if(thread_index == -1)
			{
				thread_index = last_thread_index;
				threads[thread_index] = Thread(article);
				thread_index = last_thread_index;
				last_thread_index++;
			}
			else
			{
				threads[thread_index].add(article);
			}

			article.set_thread_index(thread_index);
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
			int thread_index = articles[article_key].get_thread_index();

			if(threads.count(thread_index) != 0)
			{
				articles.erase(articles.find(article_key));
				threads[thread_index].remove(article_key);

				if(threads[thread_index].get_article_keys().size() == 0)
				{
					threads.erase(threads.find(thread_index));
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

	time_t get_last_published_time(int thread_index)
	{
		time_t last_published_time = 0;

		if(threads.count(thread_index) != 0)
		{
			for(auto article_key : threads[thread_index].get_article_keys())
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

	string get_thread_title(int thread_index)
	{
		time_t last_published_time = 0;
		wstring last_article_title(L"");

		if(threads.count(thread_index) != 0)
		{
			for(auto article_key : threads[thread_index].get_article_keys())
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

	string get_thread_category(int thread_index)
	{
		map<string, int> category_counter;
		string category("other");

		if(threads.count(thread_index) != 0)
		{
			for(auto article_key : threads[thread_index].get_article_keys())
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

	string get_thread_lang_code(int thread_index)
	{
		string lang_code("other");

		if(threads.count(thread_index) != 0)
		{
			vector<string> article_keys = threads[thread_index].get_article_keys();

			if(article_keys.size() > 0)
			{
				lang_code = articles[article_keys[0]].get_lang_code();
			}
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
	int get_most_similar_thread_index(Article& article, string lang_code, string article_key)
	{
		int thread_index = -1;
		
		set<wstring> uppercase_tokens = get_uppercase_tokens(article.get_text_tk(), lang_code);
		map<string, int> article_counter;

		for(const auto & uppercase_token : uppercase_tokens)
		{
			if(keys_by_token.count(uppercase_token) == 0)
			{
				keys_by_token[uppercase_token] = vector<string>();
			}
			else
			{
				for(const auto & other_key : keys_by_token[uppercase_token])
				{
					string other_lang_code = articles[other_key].get_lang_code();

					if(other_lang_code != lang_code)
					{
						continue;
					}

					if(article_counter.count(other_key) == 0)
					{
						article_counter[other_key] = 0;
					}

					article_counter[other_key]++;
				}
			}

			keys_by_token[uppercase_token].push_back(article_key);
		}

		int uppercase_num = uppercase_tokens.size();
		set<int> thread_indexes;
		map<string, bool> is_article_found;

		for(const auto & [other_key, token_count] : article_counter)
		{
			if(token_count >= min_similar_token_count[lang_code])
			{
				double similarity = (double)token_count / uppercase_num;

				if(similarity > threads_min_similarity[lang_code])
				{
					int thread_index_found = articles[other_key].get_thread_index();
					thread_indexes.insert(thread_index_found);
					is_article_found[other_key] = true;
				}
			}
		}

		for(auto thread_index_found : thread_indexes)
		{
			bool is_found = true;

			for(auto other_key : threads[thread_index_found].get_article_keys())
			{
				if(is_article_found.count(other_key) == 0)
				{
					is_found = false;
					break;
				}
			}

			if(is_found)
			{
				thread_index = thread_index_found;
				break;
			}
		}

		return thread_index;
	}
};

#endif