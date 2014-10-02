#ifndef LANGUAGE_BASE_H
#define LANGUAGE_BASE_H

#include <vector>
#include <unordered_map>
#include <string>

namespace Language
{
	const int unique_prefix_length = 4;
	class Base
	{
	protected:
		std::vector<std::string> *word_list;
		std::unordered_map<std::string, uint32_t> *word_map;
		std::unordered_map<std::string, uint32_t> *trimmed_word_map;
		std::string language_name;
		void populate_maps()
		{
			int ii;
			std::vector<std::string>::iterator it;
			for (it = word_list->begin(), ii = 0; it != word_list->end(); it++, ii++)
			{
				(*word_map)[*it] = ii;
				if (it->length() > 4)
				{
					(*trimmed_word_map)[it->substr(0, 4)] = ii;
				}
				else
				{
					(*trimmed_word_map)[*it] = ii;
				}
			}
		}
	public:
		Base()
		{
			word_list = new std::vector<std::string>;
			word_map = new std::unordered_map<std::string, uint32_t>;
			trimmed_word_map = new std::unordered_map<std::string, uint32_t>;
		}
		const std::vector<std::string>& get_word_list() const
		{
			return *word_list;
		}
		const std::unordered_map<std::string, uint32_t>& get_word_map() const
		{
			return *word_map;
		}
		const std::unordered_map<std::string, uint32_t>& get_trimmed_word_map() const
		{
			return *trimmed_word_map;
		}
		std::string get_language_name() const
		{
			return language_name;
		}
	};
}

#endif
