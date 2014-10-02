/*!
 * \file language_base.h
 * 
 * \brief Language Base class for Polymorphism.
 */

#ifndef LANGUAGE_BASE_H
#define LANGUAGE_BASE_H

#include <vector>
#include <unordered_map>
#include <string>

/*!
 * \namespace Language
 * \brief Mnemonic language related namespace.
 */
namespace Language
{
	const int unique_prefix_length = 4; /*!< Length of the prefix of all words guaranteed to be unique */
	/*!
	 * \class Base
	 * \brief A base language class which all languages have to inherit from for
	 * Polymorphism.
	 */
	class Base
	{
	protected:
		std::vector<std::string> *word_list; /*!< A pointer to the array of words */
		std::unordered_map<std::string, uint32_t> *word_map; /*!< hash table to find word's index */
		std::unordered_map<std::string, uint32_t> *trimmed_word_map; /*!< hash table to find word's trimmed index */
		std::string language_name; /*!< Name of language */
		/*!
		 * \brief Populates the word maps after the list is ready.
		 */
		void populate_maps()
		{
			int ii;
			std::vector<std::string>::iterator it;
			for (it = word_list->begin(), ii = 0; it != word_list->end(); it++, ii++)
			{
				(*word_map)[*it] = ii;
				if (it->length() > unique_prefix_length)
				{
					(*trimmed_word_map)[it->substr(0, unique_prefix_length)] = ii;
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
		/*!
		 * \brief Returns a pointer to the word list.
		 * \return A pointer to the word list.
		 */
		const std::vector<std::string>& get_word_list() const
		{
			return *word_list;
		}
		/*!
		 * \brief Returns a pointer to the word map.
		 * \return A pointer to the word map.
		 */
		const std::unordered_map<std::string, uint32_t>& get_word_map() const
		{
			return *word_map;
		}
		/*!
		 * \brief Returns a pointer to the trimmed word map.
		 * \return A pointer to the trimmed word map.
		 */
		const std::unordered_map<std::string, uint32_t>& get_trimmed_word_map() const
		{
			return *trimmed_word_map;
		}
		/*!
		 * \brief Returns the name of the language.
		 * \return Name of the language.
		 */
		std::string get_language_name() const
		{
			return language_name;
		}
	};
}

#endif
