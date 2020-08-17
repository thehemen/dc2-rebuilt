#include <iostream>
#include <map>
#include <utils.h>
#include <nlp_utils.h>

using namespace std;

#ifndef HTML_PARSER_H
#define HTML_PARSER_H

struct HTMLNode
{
	vector<HTMLNode> nodes;
	bool isSingle;
	wstring name;
	map<wstring, wstring> attributes;
	wstring content;

	HTMLNode(wstring name, map<wstring, wstring> attributes)
	{
		// Single node.
		this->nodes = vector<HTMLNode>();
		this->isSingle = true;
		this->name = name;
		this->attributes = attributes;
		this->content = L"";
	}

	HTMLNode(wstring name, map<wstring, wstring> attributes, wstring content)
	{
		// Open-close node with text content.
		this->nodes = vector<HTMLNode>();
		this->isSingle = false;
		this->name = name;
		this->attributes = attributes;
		this->content = content;
	}

	HTMLNode(wstring name, map<wstring, wstring> attributes, vector<HTMLNode> nodes)
	{
		// Open-close node with subnodes.
		this->nodes = nodes;
		this->isSingle = false;
		this->name = name;
		this->attributes = map<wstring, wstring>();
		this->content = L"";
	}

	vector<HTMLNode> find_by_tag(wstring name)
	{
		vector<HTMLNode> nodesFound;

		for(int i = 0, len = nodes.size(); i < len; ++i)
		{
			if(nodes[i].name == name)
			{
				nodesFound.push_back(nodes[i]);
			}

			vector<HTMLNode> slaveNodesFound = nodes[i].find_by_tag(name);
			nodesFound.insert(nodesFound.end(), slaveNodesFound.begin(), slaveNodesFound.end());
		}

		return nodesFound;
	}

	vector<HTMLNode> find_by_attr(wstring tagName, wstring attrName, wstring attrValue)
	{
		vector<HTMLNode> nodesFound;

		for(int i = 0, len = nodes.size(); i < len; ++i)
		{
			if(nodes[i].name == tagName)
			{
				if(nodes[i].attributes[attrName] == attrValue)
				{
					nodesFound.push_back(nodes[i]);
				}
			}

			vector<HTMLNode> slaveNodesFound = nodes[i].find_by_attr(tagName, attrName, attrValue);
			nodesFound.insert(nodesFound.end(), slaveNodesFound.begin(), slaveNodesFound.end());
		}

		return nodesFound;
	}

	vector<wstring> find_content()
	{
		vector<wstring> contents;

		if(content.size() > 0)
		{
			contents.push_back(content);
		}

		for(int i = 0, len = nodes.size(); i < len; ++i)
		{
			vector<wstring> slaveContents = nodes[i].find_content();
			contents.insert(contents.end(), slaveContents.begin(), slaveContents.end());
		}

		return contents;
	}
};

class HTMLParser
{
	int nodeIndex;
	int nodeCount;
	wstring rawhtml;
	vector<size_t> lessSign;
	vector<size_t> moreSign;
	vector<bool> isLessSignClosing;
	vector<bool> isMoreSignClosing;
public:
	HTMLParser(wstring rawhtml)
	{
		this->rawhtml = wstring(rawhtml);

		// replace <br/> tag with the newline character
		this->rawhtml = replace_with(this->rawhtml, L"<br/>", L"\n");

		lessSign = find_all(this->rawhtml, L"<");
		moreSign = find_all(this->rawhtml, L">");

		// Closing tag </...>
		nodeCount = lessSign.size();
		isLessSignClosing = vector<bool>(nodeCount, false);

		for(int i = 0; i < nodeCount; ++i)
		{
			if(this->rawhtml[lessSign[i] + 1] == '/')
			{
				isLessSignClosing[i] = true;
			}
		}

		// Single tag <.../>
		isMoreSignClosing = vector<bool>(nodeCount, false);

		for(int i = 0; i < nodeCount; ++i)
		{
			if(this->rawhtml[moreSign[i] - 1] == '/')
			{
				isMoreSignClosing[i] = true;
			}
		}
	}

	vector<HTMLNode> parse()
	{
		// The first two <> brackets (<DOCTYPE...>) are skipped.
		nodeIndex = 1;
		return parse_subnodes();
	}

private:
	vector<HTMLNode> parse_subnodes(wstring openTagName = L"")
	{
		vector<HTMLNode> nodes;

		while(nodeIndex < nodeCount)
		{
			if(isLessSignClosing[nodeIndex])  // </...>
			{
				auto name_attr = get_name_attr(rawhtml, lessSign[nodeIndex] + 2, moreSign[nodeIndex] - 1);
				wstring closeTagName = name_attr.first;

				if(openTagName == closeTagName)
				{
					break;
				}
				else
				{
					nodeIndex++;
				}
			}
			else if(isMoreSignClosing[nodeIndex])  // <.../>
			{
				auto name_attr = get_name_attr(rawhtml, lessSign[nodeIndex] + 1, moreSign[nodeIndex] - 2);
				nodeIndex++;
				HTMLNode node(name_attr.first, name_attr.second);
				nodes.push_back(node);
			}
			else  // <...>
			{
				auto name_attr = get_name_attr(rawhtml, lessSign[nodeIndex] + 1, moreSign[nodeIndex] - 1);
				nodeIndex++;
				vector<HTMLNode> subNodes = parse_subnodes(name_attr.first);
				
				if(subNodes.size() > 0)
				{
					// With subnodes.
					HTMLNode node(name_attr.first, name_attr.second, subNodes);
					nodes.push_back(node);
				}
				else
				{
					// With text content.
					wstring content = get_substr(rawhtml, moreSign[nodeIndex - 1] + 1, lessSign[nodeIndex] - 1);
					HTMLNode node(name_attr.first, name_attr.second, content);
					nodes.push_back(node);
				}
			}
		}

		return nodes;
	}

	pair<wstring, map<wstring, wstring>> get_name_attr(wstring rawhtml, size_t beginIndex, size_t endIndex)
	{
		wstring rawname = get_substr(rawhtml, beginIndex, endIndex);
		wstring name;
		map<wstring, wstring> attributes;

		size_t sep = rawname.find(L" ");
		if(sep != string::npos)
		{
			name = get_substr(rawname, 0, sep - 1);
			attributes = get_attributes(rawname, sep);
		}
		else
		{
			name = rawname;
		}

		return pair<wstring, map<wstring, wstring>>(name, attributes);
	}

	map<wstring, wstring> get_attributes(wstring rawname, size_t sep)
	{
		map<wstring, wstring> attributes;

		// Get all quote mark positions and make mask by quotes
		wstring attrs = get_substr(rawname, sep + 1, rawname.size() - 1);
		size_t attrLen = attrs.size();
		vector<size_t> quotes = find_all(attrs, L"\"");
		vector<bool> quoteMask(attrLen, false);

		for(int i = 0, quoteLen = quotes.size() / 2; i < quoteLen; ++i)
		{
			int quoteBegin = quotes[i * 2];
			int quoteEnd = quotes[i * 2 + 1];
			fill(quoteMask.begin() + quoteBegin, quoteMask.begin() + quoteEnd, true);
		}

		// Find all spaces outside quotes
		vector<size_t> spaces = find_all(attrs, L" ");
		vector<size_t> substrBounds;

		for(int i = 0, spaceLen = spaces.size(); i < spaceLen; ++i)
		{
			if(!quoteMask[spaces[i]])
			{
				substrBounds.push_back(spaces[i]);
			}
		}

		// Needed to extract all the attributes.
		substrBounds.insert(substrBounds.begin(), -1);
		substrBounds.push_back(attrLen);

		// Extract attribute substring and assign to map by <name, value>
		for(int i = 1, spaceLen = substrBounds.size(); i < spaceLen; ++i)
		{
			int substrBegin = substrBounds[i - 1] + 1;
			int substrLen = substrBounds[i] - substrBegin + 1;
			wstring attr = attrs.substr(substrBegin, substrLen);
			int delimPos = attr.find_first_of(L"=");
			wstring attrName = attr.substr(0, delimPos);
			vector<size_t> quotesInline = find_all(attr, L"\"");

			if(quotesInline.size() > 1)
			{
				wstring attrValue = get_substr(attr, quotesInline[0] + 1, quotesInline[1] - 1);  // without quote marks
				attributes[attrName] = attrValue;
			}
		}

		return attributes;
	}
};

class HTMLDocument
{
	vector<HTMLNode> nodes;
public:
	HTMLDocument() {}

	HTMLDocument(const char* filename)
	{
		wstring rawhtml = read_file(filename);
		HTMLParser htmlParser(rawhtml);
		nodes = htmlParser.parse();
	}

	HTMLDocument(wstring rawhtml)
	{
		HTMLParser htmlParser(rawhtml);
		nodes = htmlParser.parse();
	}

	time_t get_published_time()
	{
		auto nodes = find_by_attr(L"meta", L"property", L"article:published_time");

		if(nodes.size() > 0)
		{
			wstring datetime_raw = nodes[0].attributes[L"content"];
			return extract_datetime(datetime_raw);
		}
		else
		{
			return 0;
		}
	}

	wstring get_short_url()
	{
		auto nodes = find_by_attr(L"meta", L"property", L"og:url");

		if(nodes.size() > 0)
		{
			wstring url_long = nodes[0].attributes[L"content"];
			vector<wstring> url_parts = tokenize(url_long, L'/');

			if(url_parts.size() > 1)
			{
				return url_parts[1];
			}
			else
			{
				return L"";
			}
		}
		else
		{
			return L"";
		}
	}

	wstring get_header()
	{
		auto nodes = find_by_tag(L"h1");

		if(nodes.size() > 0)
		{
			wstring header = nodes[0].content;
			return header;
		}
		else
		{
			return L"";
		}
	}

	wstring get_text()
	{
		auto nodes = find_by_tag(L"p");
		wstring text;

		for(int i = 0, len = nodes.size(); i < len; ++i)
		{
			for(const auto & content : nodes[i].find_content())
			{
				text += content + L"\n";
			}
		}

		return text;
	}

	vector<HTMLNode> find_by_tag(wstring name)
	{
		if(is_valid())
		{
			return nodes[0].find_by_tag(name);
		}
		else
		{
			return vector<HTMLNode>();
		}
	}

	vector<HTMLNode> find_by_attr(wstring tagName, wstring attrName, wstring attrValue)
	{
		if(is_valid())
		{
			return nodes[0].find_by_attr(tagName, attrName, attrValue);
		}
		else
		{
			return vector<HTMLNode>();
		}
	}

	bool is_valid()
	{
		// The only head tag must be <head> (not including <DOCTYPE...>).
		return nodes.size() > 0;
	}
};

#endif