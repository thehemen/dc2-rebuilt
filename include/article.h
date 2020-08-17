#include <html_parser.h>
#include <nlp_utils.h>

#ifndef ARTICLE_H
#define ARTICLE_H

class Article
{
	time_t published_time;
	wstring short_url;
	wstring header_raw;
	vector<wstring> header_tk;
	vector<wstring> text_tk;
public:
	Article(const char* filename)
	{
		HTMLDocument htmlDocument(filename);
		published_time = htmlDocument.get_published_time();
		short_url = htmlDocument.get_short_url();
		header_raw = htmlDocument.get_header();
		wstring text_raw = htmlDocument.get_text();

		header_tk = tokenize_with_punctuation(header_raw);
		text_tk = tokenize_with_punctuation(text_raw);
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
};

#endif