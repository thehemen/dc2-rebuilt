#include <nlp_utils.h>

#ifndef LANG_DETECTOR_H
#define LANG_DETECTOR_H

class LanguageDetector
{
	double word_share;
	double en_common_share;

	vector<wstring> en_common_words;
	vector<wchar_t> ru_vowels;
	wchar_t ru_hardsign;
public:
	LanguageDetector(double word_share, double en_common_share)
	{
		this->en_common_words = {L"an", L"and", L"in", L"of", L"on", L"the", L"that", L"to", L"is"};
		this->ru_vowels = {L'а', L'о', L'э', L'и', L'у', L'ы', L'е', L'ё', L'ю', L'я'};
		this->ru_hardsign = L'ъ';

		this->word_share = word_share;
		this->en_common_share = en_common_share;
	}

	string detect(vector<wstring> tokens)
	{
		string lang_code("other");
		int en_count = 0;
		int ru_count = 0;
		int all_count = 0;

		for(const auto & token : tokens)
		{
			TokenType tokenType = get_token_type(token);

			if(tokenType == TokenType::Latin)
			{
				en_count++;
			}
			else if(tokenType == TokenType::Cyrillic)
			{
				ru_count++;
			}

			if(tokenType != TokenType::Number && tokenType != TokenType::Punctuation)
			{
				all_count++;
			}
		}

		int min_share = (double)all_count * word_share;

		if(en_count > min_share)
		{
			if(is_language_en(tokens, all_count))
			{
				lang_code = "en";
			}
		}
		else if((en_count + ru_count) > min_share)
		{
			if(is_language_ru(tokens))
			{
				lang_code = "ru";
			}
		}

		return lang_code;
	}

	bool is_language_en(vector<wstring> tokens, int all_count)
	{
		int en_count = 0;

		for(const auto & token: tokens)
		{
			if(find(en_common_words.begin(), en_common_words.end(), token) != en_common_words.end())
			{
				++en_count;
			}
		}

		if(((double)en_count / all_count) > en_common_share)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool is_language_ru(vector<wstring> tokens)
	{
		bool is_not_found = true;  // is a hard sign between consonants not found

		for(const auto & token: tokens)
		{
			size_t index = token.find(ru_hardsign, 0);

			if(index != string::npos)
			{
				if(index > 0 && index < token.size() - 1)
				{
					bool is_vowel_first = find(ru_vowels.begin(), ru_vowels.end(), token[index - 1]) != ru_vowels.end();
					bool is_vowel_second = find(ru_vowels.begin(), ru_vowels.end(), token[index + 1]) != ru_vowels.end();

					if(!is_vowel_first && !is_vowel_second)
					{
						is_not_found = false;
						break;
					}
				}
			}
		}

		return is_not_found;
	}
};

#endif