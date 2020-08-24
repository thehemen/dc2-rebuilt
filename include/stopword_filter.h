#include <vector>
#include <map>
#include <utils.h>
#include <nlp_utils.h>

#ifndef STOPWORDS_FILTER_H
#define STOPWORDS_FILTER_H

using namespace std;

class StopwordFilter
{
	map<string, vector<wstring>> stopwords;
public:
	StopwordFilter() {}
	StopwordFilter(string en_filename, string ru_filename)
	{
		stopwords["en"] = read_stopwords(en_filename);
		stopwords["ru"] = read_stopwords(ru_filename);
	}

	vector<wstring> filter_stopwords(vector<wstring> tokens, string lang_code)
	{
		vector<wstring> filtered_tokens;

		for(const auto & token : tokens)
		{
			TokenType tokenType = get_token_type(token);

			// Numbers are allowerd.
			if(tokenType != TokenType::Number)
			{
				if(lang_code == "en")
				{
					if(tokenType != TokenType::Latin)
					{
						continue;
					}
				}
				else if(lang_code == "ru")
				{
					// Both latin and cyrillic words are allowed for RU.
					if(tokenType != TokenType::Latin && tokenType != TokenType::Cyrillic)
					{
						continue;
					}
				}

				bool is_stopword = false;

				for(const auto &stopword : stopwords[lang_code])
				{
					if(token == stopword)
					{
						is_stopword = true;
						break;
					}
				}

				if(is_stopword)
				{
					continue;
				}
			}

			filtered_tokens.push_back(token);
		}

		return filtered_tokens;
	}

private:
	vector<wstring> read_stopwords(string filename)
	{
		vector<wstring> stopwords;
		wstringstream text(read_file(filename.c_str()));
		wstring line;

		while (getline(text, line))
		{
			stopwords.push_back(line);
		}

		return stopwords;
	}
};

#endif