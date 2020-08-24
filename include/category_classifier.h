#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <utils.h>
#include <nlp_utils.h>

#ifndef CATEGORY_CLASSIFIER_H
#define CATEGORY_CLASSIFIER_H

using namespace std;

class CategoryClassifier
{
	set<string> categories;
	map<string, map<wstring, string>> keywords;
	double min_char_share;
	map<string, int> min_token_count;
public:
	CategoryClassifier() {}

	CategoryClassifier(string en_filename, string ru_filename, double min_char_share, map<string, int> min_token_count)
	{
		categories = set<string>();
		keywords["en"] = read_keywords(en_filename, "en");
		keywords["ru"] = read_keywords(ru_filename, "ru");
		this->min_char_share = min_char_share;
		this->min_token_count = min_token_count;
	}

	string classify(vector<wstring> tokens, string lang_code)
	{
		map<string, int> counter;

		for(const auto & category : categories)
		{
			counter[category] = 0;
		}

		for(const auto & token : tokens)
		{
			TokenType tokenType = get_token_type(token);

			// Skip all non-latin (for EN) and non-cyrillic (for RU) tokens.
			if(lang_code == "en" && tokenType != TokenType::Latin)
			{
				continue;
			}
			else if(lang_code == "ru" && tokenType != TokenType::Cyrillic)
			{
				continue;
			}

			int token_len = token.size();
			double min_token_share = (double)token_len * min_char_share;
			bool is_found = false;
			string category_found("other");

			for(const auto & [keyword, category] : keywords[lang_code])
			{
				if(min_token_share <= keyword.size())
				{
					if(token.rfind(keyword, 0) == 0)
					{
						category_found = keywords[lang_code][keyword];
						is_found = true;
						break;
					}
				}
			}

			if(is_found)
			{
				counter[category_found]++;
			}
		}

		auto category_num = get_pair_by_max_value(counter);
		string category_best = get<0>(category_num);
		int category_count = get<1>(category_num);

		if(category_count > min_token_count[lang_code])
		{
			return category_best;
		}
		else
		{
			return "other";
		}
	}

private:
	map<wstring, string> read_keywords(string filename, string lang_code)
	{
		map<wstring, string> category_by_keyword;
		wstring raw_data = read_file(filename.c_str());
		wstringstream wstream(raw_data);
		wstring line;
		string category_now;

		while(getline(wstream, line))
		{
			vector<wstring> tokens = tokenize(line);

			if(tokens.size() == 1)
			{
				category_now = wstring_to_utf8(tokens[0]);
			}
			else
			{
				make_stemming(lang_code, tokens);

				for(const auto & token : tokens)
				{
					categories.insert(category_now);
					category_by_keyword[token] = category_now;
				}
			}
		}

		return category_by_keyword;
	}
};

#endif