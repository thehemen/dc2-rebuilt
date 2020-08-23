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
	int thread_index;
public:
	Article() {}

	Article(const char* path, string filename_only)
	{
		this->filename_only = filename_only;

		HTMLDocument htmlDocument(path);
		published_time = htmlDocument.get_published_time();
		short_url = htmlDocument.get_short_url();
		header_raw = htmlDocument.get_header();
		wstring text_raw = htmlDocument.get_text();

		header_tk = tokenize_with_punctuation(header_raw);
		text_tk = tokenize_with_punctuation(text_raw);

		lang_code = "other";
		thread_index = -1;
	}

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

	vector<wstring> get_text_tk()
	{
		return text_tk;
	}

	string get_filename_only()
	{
		return filename_only;
	}

	void set_thread_index(int thread_index)
	{
		this->thread_index = thread_index;
	}

	int get_thread_index()
	{
		return thread_index;
	}

	void set_lang_code(string lang_code)
	{
		this->lang_code = lang_code;
	}

	string get_lang_code()
	{
		return lang_code;
	}
};

#endif