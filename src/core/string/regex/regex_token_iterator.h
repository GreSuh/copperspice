/***********************************************************************
*
* Copyright (c) 2017-2023 Barbara Geller
* Copyright (c) 2017-2023 Ansel Sermersheim
*
* Copyright (c) 1998-2009 John Maddock
*
* This file is part of CopperSpice.
*
* CopperSpice is free software, released under the BSD 2-Clause license.
* For license details refer to LICENSE provided with this project.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://opensource.org/licenses/BSD-2-Clause
*
***********************************************************************/

/*
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef CS_REGEX_TOKEN_ITERATOR_H
#define CS_REGEX_TOKEN_ITERATOR_H

namespace cs_regex_ns {

template <class BidirectionalIterator, class charT, class traits>
class regex_token_iterator_implementation
{
   typedef basic_regex<charT, traits> regex_type;
   typedef sub_match<BidirectionalIterator>      value_type;

   match_results<BidirectionalIterator> what;   // current match
   BidirectionalIterator                base;   // start of search area
   BidirectionalIterator                end;    // end of search area
   const regex_type                     re;     // the expression
   match_flag_type                      flags;  // match flags
   value_type                           result; // the current string result
   int                                  N;      // the current sub-expression being enumerated
   std::vector<int>                     subs;   // the sub-expressions to enumerate

 public:
   regex_token_iterator_implementation(const regex_type *p, BidirectionalIterator last, int sub, match_flag_type f)
      : end(last), re(*p), flags(f) {
      subs.push_back(sub);
   }

   regex_token_iterator_implementation(const regex_type *p, BidirectionalIterator last, const std::vector<int> &v, match_flag_type f)
      : end(last), re(*p), flags(f), subs(v) {}

   template <std::size_t CN>
   regex_token_iterator_implementation(const regex_type *p, BidirectionalIterator last, const int (&submatches)[CN], match_flag_type f)
      : end(last), re(*p), flags(f) {
      for (std::size_t i = 0; i < CN; ++i) {
         subs.push_back(submatches[i]);
      }
   }

   bool init(BidirectionalIterator first) {
      N = 0;
      base = first;
      if (regex_search(first, end, what, re, flags, base) == true) {
         N = 0;
         result = ((subs[N] == -1) ? what.prefix() : what[(int)subs[N]]);
         return true;
      } else if ((subs[N] == -1) && (first != end)) {
         result.first = first;
         result.second = end;
         result.matched = (first != end);
         N = -1;
         return true;
      }
      return false;
   }

   bool compare(const regex_token_iterator_implementation &that) {
      if (this == &that) {
         return true;
      }

      return (&re.get_data() == &that.re.get_data())
             && (end == that.end)
             && (flags == that.flags)
             && (N == that.N)
             && (what[0].first == that.what[0].first)
             && (what[0].second == that.what[0].second);
   }

   const value_type &get() {
      return result;
   }

   bool next() {
      if (N == -1) {
         return false;
      }

      if (N + 1 < (int)subs.size()) {
         ++N;
         result = ((subs[N] == -1) ? what.prefix() : what[subs[N]]);
         return true;
      }

      BidirectionalIterator last_end(what[0].second);
      if (regex_search(last_end, end, what, re, ((what[0].first == what[0].second) ? flags | regex_constants::match_not_initial_null : flags), base)) {
         N = 0;
         result = ((subs[N] == -1) ? what.prefix() : what[subs[N]]);
         return true;
      } else if ((last_end != end) && (subs[0] == -1)) {
         N = -1;
         result.first = last_end;
         result.second = end;
         result.matched = (last_end != end);
         return true;
      }
      return false;
   }

 private:
   regex_token_iterator_implementation &operator=(const regex_token_iterator_implementation &);
};

template <class BidirectionalIterator, class charT, class traits>
class regex_token_iterator
{
 private:
   typedef regex_token_iterator_implementation<BidirectionalIterator, charT, traits> impl;
   typedef std::shared_ptr<impl> pimpl;

 public:
   using regex_type        = basic_regex<charT, traits>;
   using difference_type   = typename cs_regex_detail_ns::regex_iterator_traits<BidirectionalIterator>::difference_type;
   using value_type        = sub_match<BidirectionalIterator>;
   using iterator_category = std::forward_iterator_tag;
   using pointer           = const value_type *;
   using reference         = const value_type &;

   regex_token_iterator() {}
   regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type &re,
                        int submatch = 0, match_flag_type m = match_default)
      : pdata(new impl(&re, b, submatch, m))   {

      if (!pdata->init(a)) {
         pdata.reset();
      }
   }
   regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type &re,
                        const std::vector<int> &submatches, match_flag_type m = match_default)
      : pdata(new impl(&re, b, submatches, m)) {
      if (! pdata->init(a)) {
         pdata.reset();
      }
   }

   template <std::size_t N>
   regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type &re,
                        const int (&submatches)[N], match_flag_type m = match_default)
      : pdata(new impl(&re, b, submatches, m)) {
      if (!pdata->init(a)) {
         pdata.reset();
      }
   }

   regex_token_iterator(const regex_token_iterator &that)
      : pdata(that.pdata) {}

   regex_token_iterator &operator=(const regex_token_iterator &that) {
      pdata = that.pdata;
      return *this;
   }

   bool operator==(const regex_token_iterator &that) const {
      if ((pdata.get() == 0) || (that.pdata.get() == 0)) {
         return pdata.get() == that.pdata.get();
      }
      return pdata->compare(*(that.pdata.get()));
   }

   bool operator!=(const regex_token_iterator &that) const {
      return !(*this == that);
   }

   const value_type &operator*() const {
      return pdata->get();
   }

   const value_type *operator->() const {
      return &(pdata->get());
   }

   regex_token_iterator &operator++() {
      cow();
      if (0 == pdata->next()) {
         pdata.reset();
      }
      return *this;
   }
   regex_token_iterator operator++(int) {
      regex_token_iterator result(*this);
      ++(*this);
      return result;
   }

 private:
   pimpl pdata;

   void cow() {
      if (pdata.get() && !pdata.unique()) {
         pdata.reset(new impl(*(pdata.get())));
      }
   }
};

template <class charT, class traits>
inline regex_token_iterator<const charT *, charT, traits> make_regex_token_iterator(const charT *p, const basic_regex<charT, traits> &e,
      int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<const charT *, charT, traits>(p, p + traits::length(p), e, submatch, m);
}

template <class charT, class traits, class ST, class SA>
inline regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits> make_regex_token_iterator(
   const std::basic_string<charT, ST, SA> &p, const basic_regex<charT, traits> &e, int submatch = 0,
   regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits>(p.begin(), p.end(), e, submatch, m);
}

template <class charT, class traits, std::size_t N>
inline regex_token_iterator<const charT *, charT, traits> make_regex_token_iterator(const charT *p, const basic_regex<charT, traits> &e,
      const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<const charT *, charT, traits>(p, p + traits::length(p), e, submatch, m);
}

template <class charT, class traits, class ST, class SA, std::size_t N>
inline regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits> make_regex_token_iterator(
   const std::basic_string<charT, ST, SA> &p, const basic_regex<charT, traits> &e, const int (&submatch)[N],
   regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits>(p.begin(), p.end(), e, submatch, m);
}

template <class charT, class traits>
inline regex_token_iterator<const charT *, charT, traits> make_regex_token_iterator(const charT *p, const basic_regex<charT, traits> &e,
      const std::vector<int> &submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<const charT *, charT, traits>(p, p + traits::length(p), e, submatch, m);
}

template <class charT, class traits, class ST, class SA>
inline regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits> make_regex_token_iterator(
   const std::basic_string<charT, ST, SA> &p, const basic_regex<charT, traits> &e, const std::vector<int> &submatch,
   regex_constants::match_flag_type m = regex_constants::match_default)
{
   return regex_token_iterator<typename std::basic_string<charT, ST, SA>::const_iterator, charT, traits>(p.begin(), p.end(), e, submatch, m);
}


} // namespace

#endif



