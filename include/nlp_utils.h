#include <vector>
#include <map>
#include <set>
#include <cwctype>
#include <algorithm>
#include <utils.h>

#ifndef NLP_UTILS_H
#define NLP_UTILS_H

vector<wstring> tokenize(wstring text, wchar_t delimiter = L' ')
{
    vector<wstring> tokens;
    size_t pos = text.find(delimiter);
    size_t initialPos = 0;

    while(pos != wstring::npos)
    {
        tokens.push_back(text.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;
        pos = text.find(delimiter, initialPos);
    }

    tokens.push_back(text.substr(initialPos, min(pos, text.size()) - initialPos + 1));

    // remove empty tokens
    for(int i = tokens.size() - 1; i >= 0; --i)
    {
    	if(tokens[i].size() == 0)
    	{
    		tokens.erase(tokens.begin() + i);
    	}
    }
    return tokens;
}

vector<wstring> tokenize_with_punctuation(wstring text)
{
	vector<wstring> tokens;
	vector<wchar_t> punct_signs = { L'.', L',', L':', L';', L'!', L'?', L'\'', L'\"', L'“', L'«', L'»', L'-' };
	vector<size_t> positions;

	wstring nonewlines = replace_with(text, L"\n", L" ");

	for(const auto & sign : punct_signs)
	{
		vector<size_t> temp_pos = get_all_occurences(nonewlines, sign);
		positions.insert(positions.end(), temp_pos.begin(), temp_pos.end());
	}
	sort(positions.begin(), positions.end(), greater<int>());

	for(const auto & pos : positions)
	{
		nonewlines.insert(pos + 1, 1, L' ');
		nonewlines.insert(pos, 1, L' ');
	}

	return tokenize(nonewlines);
}

enum TokenType { Other, Latin, Cyrillic, Number, Punctuation };

TokenType get_token_type(wstring token)
{
	int en_count = 0;
	int ru_count = 0;
	int digit_count = 0;
	int text_len = token.size();

	if(text_len == 1)
	{
		if(::iswpunct(token[0]))
		{
			return TokenType::Punctuation;
		}
	}

	for(const auto & c : token)
	{
		wchar_t cl = ::towlower(c);

		if(cl >= L'a' && cl <= L'z')
		{
			en_count++;
		}
		else if(cl >= L'а' && cl <= L'я')
		{
			ru_count++;
		}
		else if(cl >= L'0' && cl <= L'9')
		{
			digit_count++;
		}
	}

	if(text_len == en_count)
	{
		return TokenType::Latin;
	}
	else if(text_len == ru_count)
	{
		return TokenType::Cyrillic;
	}
	else if(text_len == digit_count)
	{
		return TokenType::Number;
	}
	else
	{
		return TokenType::Other;
	}
}

set<wstring> get_uppercase_tokens(vector<wstring> tokens, string lang_code)
{
	map<string, wchar_t> start_letter = {{"en", L'A'}, {"ru", L'А'}};
	map<string, wchar_t> end_letter = {{"en", L'Z'}, {"ru", L'Я'}};
	vector<wchar_t> sentence_end_chars = { L'.', L'!', L'?', L'\"', L'“', L'«' };

	set<wstring> uppercase_tokens;
	bool is_sentence_start = true;

	vector<wstring> uppercase_by_sentence;
	int sentence_len = 0;

	for(const auto & token : tokens)
	{
		if(token.size() == 0)
		{
			continue;
		}

		TokenType tokenType = get_token_type(token);

		if(is_sentence_start)
		{
			if(tokenType != TokenType::Punctuation)
			{
				is_sentence_start = false;
				continue;
			}
		}

		wchar_t start_character = token[0];

		// A pronoun "I" is always uppercase in English.
		if(lang_code == "en" && token.size() == 1 && start_character == L'I')
		{
			continue;
		}

		if(tokenType == TokenType::Latin || tokenType == TokenType::Cyrillic)
		{
			if(start_character >= start_letter["en"] && start_character <= end_letter["en"] ||
				start_character >= start_letter["ru"] && start_character <= end_letter["ru"])
			{
				uppercase_by_sentence.push_back(token);
			}
		}
		else if(tokenType == TokenType::Punctuation)
		{
			for(wchar_t sentence_end_char : sentence_end_chars)
			{
				if(start_character == sentence_end_char)
				{
					// Add uppercase tokens only if they don't fill all the sentence.
					int uppercase_num = uppercase_by_sentence.size();

					if(uppercase_num < sentence_len)
					{
						uppercase_tokens.insert(uppercase_by_sentence.begin(),
							uppercase_by_sentence.end());
					}

					uppercase_by_sentence.clear();
					is_sentence_start = true;
					sentence_len = 0;
					break;
				}
			}
		}

		if(!is_sentence_start)
		{
			sentence_len++;
		}
	}

	return uppercase_tokens;
}

#endif