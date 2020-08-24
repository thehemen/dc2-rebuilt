#include <vector>
#include <set>
#include <html_parser.h>
#include <nlp_utils.h>

#ifndef ARTICLE_H
#define ARTICLE_H

class Article
{
	string filename_only;

	time_t published_time;
	wstring short_url;
	wstring header_raw;
	vector<wstring> header_tk;
	vector<wstring> text_tk;

	string lang_code;
	string category;
	int thread_id;
public:
	Article() {}

	Article(const char* path, string filename_only)
	{
		this->filename_only = filename_only;
		HTMLDocument htmlDocument(path);
		parse(htmlDocument);
	}

	Article(wstring content, string filename_only)
	{
		this->filename_only = filename_only;
		HTMLDocument htmlDocument(content);
		parse(htmlDocument);
	}

private:
	void parse(HTMLDocument &htmlDocument)
	{
		published_time = htmlDocument.get_published_time();
		short_url = htmlDocument.get_short_url();
		header_raw = htmlDocument.get_header();
		wstring text_raw = htmlDocument.get_text();

		header_tk = tokenize_with_punctuation(header_raw);
		text_tk = tokenize_with_punctuation(text_raw);

		// Make the header tokens lower to be compared for threads.
		for(int i = 0, len = header_tk.size(); i < len; ++i)
		{
			tolower(header_tk[i]);
		}

		lang_code = "other";
		category = "other";
		thread_id = -1;
	}

public:

	time_t get_published_time()
	{
		return published_time;
	}

	wstring get_short_url()
	{
		return short_url;
	}

	wstring get_header_raw()
	{
		return header_raw;
	}

	vector<wstring> get_header_tk()
	{
		return header_tk;
	}

	void update_header_tk(vector<wstring> header_tk)
	{
		this->header_tk = header_tk;
	}

	vector<wstring> get_text_tk()
	{
		return text_tk;
	}

	string get_filename_only()
	{
		return filename_only;
	}

	void set_thread_id(int thread_id)
	{
		this->thread_id = thread_id;
	}

	int get_thread_id()
	{
		return thread_id;
	}

	void set_lang_code(string lang_code)
	{
		make_stemming(lang_code, text_tk);
		make_stemming(lang_code, header_tk);
		this->lang_code = lang_code;
	}

	string get_lang_code()
	{
		return lang_code;
	}

	void set_category(string category)
	{
		this->category = category;
	}

	string get_category()
	{
		return category;
	}
};

#endif