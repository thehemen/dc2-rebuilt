#include <vector>
#include <map>
#include <nlp_utils.h>

#ifndef NEWS_DETECTOR_H
#define NEWS_DETECTOR_H

using namespace std;

class NewsDetector
{
	vector<wchar_t> quote_marks;
	map<string, vector<wstring>> stopwords;
	map<string, vector<wstring>> stopwords_first;
public:
	NewsDetector()
	{
		quote_marks = { L'\"', L'“', L'«', L'»' };
		stopwords = {{"ru", vector<wstring>()}, {"en", vector<wstring>()}};
		stopwords_first = {{"ru", vector<wstring>()}, {"en", vector<wstring>()}};

		stopwords["ru"] = vector<wstring>{ L"вы", L"вас", L"вам", L"вами", L"ваш", L"вашего", L"вашему", L"вашим", L"вашем" };
		stopwords["en"] = vector<wstring>{ L"you", L"your", L"your" };
		stopwords_first["ru"] =  vector<wstring>{ L"топ", L"самый", L"самая", L"самые", L"лучший", L"лучшая", L"лучшие", L"как", L"кто", L"когда", L"где", L"зачем", L"почему" };
		stopwords_first["en"] = vector<wstring>{ L"top", L"best", L"how", L"who", L"when", L"where", L"why" };
	}

	bool is_news(vector<wstring> header_tokens, string lang_code)
	{
		bool is_news_flag = true;
		bool is_quote_now = false;
		bool is_first_found = false;

		for(const auto & token : header_tokens)
		{
			TokenType tokenType = get_token_type(token);

			if(tokenType == TokenType::Punctuation)
			{
				wchar_t sign = token[0];

				for(const auto & quote_mark : quote_marks)
				{
					if(sign == quote_mark)
					{
						if(!is_quote_now)
						{
							is_quote_now = true;
						}
						else
						{
							is_quote_now = false;
						}

						break;
					}
				}

				/*
					The question and explamation marks outside the quote
					are the features of an emotional header that isn't
					suitable for true news.
				*/

				if(!is_quote_now)
				{
					if(sign == L'!' || sign == L'?')
					{
						is_news_flag = false;
						break;
					}
				}
			}
			else
			{
				/*
					The words inside someone's quote aren't considered.
				*/
				if(is_quote_now)
				{
					continue;
				}

				/*
					It's guaranteed that every token has non-zero length.
				*/

				wstring ltoken(token);
				ltoken[0] = ::towlower(token[0]);

				/*
					The direct appeal to the reader by "you" is the feature
					of an subjective article; news must be objective.
				*/

				for(const auto & stopword : stopwords[lang_code])
				{
					if(ltoken == stopword)
					{
						is_news_flag = false;
						break;
					}
				}

				if(!is_news_flag)
				{
					break;
				}

				if(!is_first_found)
				{
					if(tokenType == TokenType::Latin && lang_code == "en")
					{
						is_first_found = true;
					}
					else if(tokenType == TokenType::Cyrillic && lang_code == "ru")
					{
						is_first_found = true;
					}

					/*
						If the first word is the question word (when, how etc.) or
						some another special word (top, best), the article isn't news.
					*/

					if(is_first_found)
					{
						for(const auto & stopword : stopwords_first[lang_code])
						{
							if(ltoken == stopword)
							{
								is_news_flag = false;
								break;
							}
						}
					}
				}

				if(!is_news_flag)
				{
					break;
				}
			}
		}

		return is_news_flag;
	}
};

#endif