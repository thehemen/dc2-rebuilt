#include <vector>
#include <map>
#include <set>
#include <cwctype>
#include <algorithm>
#include <string>
#include <utils.h>

#include <olestem/stemming/russian_stem.h>
#include <olestem/stemming/english_stem.h>

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

void make_stemming(string lang_code, vector<wstring>& words)
{
	if(lang_code == "en")
	{
		stemming::english_stem<> englishStemmer;

		for(int i = 0, len = words.size(); i < len; ++i)
		{
			englishStemmer(words[i]);
		}
	}
	else if(lang_code == "ru")
	{
		stemming::russian_stem<> russianStemmer;

		for(int i = 0, len = words.size(); i < len; ++i)
		{
			russianStemmer(words[i]);
		}
	}
}

void tolower(wstring& token)
{
	transform(token.begin(), token.end(), token.begin(), [](unsigned wchar_t c)
		{ 
			return ::towlower(c); 
		}
	);
}

#endif