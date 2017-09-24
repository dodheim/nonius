// cpptempl
// =================
// This is a template engine for C++.
// 
// Syntax
// =================
// Variables: {$variable_name}
// Loops: {% for person in people %}Name: {$person.name}{% endfor %}
// If: {% for person.name == "Bob" %}Full name: Robert{% endif %}
// 
// Copyright
// ==================
// Copyright (c) Ryan Ginstrom
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// 
// Modified by: Martinho Fernandes, Adam Merz
// 
// Usage
// =======================
//  std::string text = "{% if item %}{$item}{% endif %}\n"
//      "{% if thing %}{$thing}{% endif %}";
//  cpptempl::data_map data;
//  data["item"] = cpptempl::make_data("aaa");
//  data["thing"] = cpptempl::make_data("bbb");
// 
//  std::string result = cpptempl::parse(text, data);
// 
// Handy Functions
// ========================
// make_data() : Feed it a string, data_map, or data_list to create a data entry.
// Example:
//  data_map person;
//  person["name"] = make_data("Bob");
//  person["occupation"] = make_data("Plumber");
//  data_map data;
//  data["person"] = make_data(std::move(person));
// 
//  std::string result = parse(templ_text, data);

#ifndef CPPTEMPL_H
#define CPPTEMPL_H

#ifdef _MSC_VER
#pragma warning( disable : 4996 ) // 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#pragma warning( disable : 4512 ) // 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif

#include <nonius/detail/noexcept.h++>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include <ostream>
#include <sstream>

#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace cpptempl {
    // various typedefs

    // data classes
    struct data_map;
    struct Data;
    struct DataValue;
    struct DataList;
    struct DataMap;

    struct data_ptr {
        data_ptr() NONIUS_NOEXCEPT = default;
        data_ptr(const data_ptr&) NONIUS_NOEXCEPT = default;
        data_ptr(data_ptr&&) NONIUS_NOEXCEPT = default;
        template<typename T, typename std::enable_if<!std::is_same<T, data_ptr>::value, int>::type = 0>
        data_ptr(const T& data) {
            *this = data;
        }
        data_ptr(std::shared_ptr<DataValue> data) NONIUS_NOEXCEPT;
        data_ptr(std::shared_ptr<DataList> data) NONIUS_NOEXCEPT;
        data_ptr(std::shared_ptr<DataMap> data) NONIUS_NOEXCEPT;

        data_ptr& operator =(const data_ptr&) NONIUS_NOEXCEPT = default;
        data_ptr& operator =(data_ptr&&) NONIUS_NOEXCEPT = default;
        template<typename T, typename std::enable_if<!std::is_same<T, data_ptr>::value, int>::type = 0>
        data_ptr& operator =(const T& data);
        data_ptr& operator =(std::string data);
        data_ptr& operator =(data_map data);

        template<typename T, typename std::enable_if<std::is_constructible<data_ptr, T&&>::value, int>::type = 0>
        data_ptr& emplace_back(T&& data);
        void push_back(data_ptr data) {
            emplace_back(std::move(data));
        }

        virtual ~data_ptr() NONIUS_NOEXCEPT = default;
        Data* operator ->() NONIUS_NOEXCEPT {
            return ptr.get();
        }
    private:
        std::shared_ptr<Data> ptr;
    };
    using data_list = std::vector<data_ptr>;

    struct data_map {
        data_ptr& operator [](const std::string& key);
        bool empty() const NONIUS_NOEXCEPT;
        bool has(const std::string& key) const;
    private:
        std::unordered_map<std::string, data_ptr> data;
    };

    template<typename T, typename std::enable_if<!std::is_same<T, data_ptr>::value, int>::type>
    data_ptr& data_ptr::operator =(const T& data) {
        std::string data_str = boost::lexical_cast<std::string>(data);
        return *this = std::move(data_str);
    }

    // token classes
    struct Token;
    using token_ptr = std::shared_ptr<Token>;
    using token_vector = std::vector<token_ptr>;

    // Custom exception class for library errors
    struct TemplateException : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    // Data types used in templates
    struct Data {
        virtual bool empty() const NONIUS_NOEXCEPT = 0;
        virtual std::string& getvalue();
        virtual data_list& getlist();
        virtual data_map& getmap();
    };

    struct DataValue : Data {
        DataValue(std::string value) NONIUS_NOEXCEPT : m_value(std::move(value)) {}
        std::string& getvalue() override;
        bool empty() const NONIUS_NOEXCEPT override;
    private:
        std::string m_value;
    };

    struct DataList : Data {
        DataList(data_list items) : m_items(std::move(items)) {}
        data_list& getlist() override;
        bool empty() const NONIUS_NOEXCEPT override;
    private:
        data_list m_items;
    };

    struct DataMap : Data {
        DataMap(data_map items) : m_items(std::move(items)) {}
        data_map& getmap() override;
        bool empty() const NONIUS_NOEXCEPT override;
    private:
        data_map m_items;
    };

    inline data_ptr::data_ptr(std::shared_ptr<DataValue> data) NONIUS_NOEXCEPT : ptr(std::move(data)) {}
    inline data_ptr::data_ptr(std::shared_ptr<DataList> data) NONIUS_NOEXCEPT : ptr(std::move(data)) {}
    inline data_ptr::data_ptr(std::shared_ptr<DataMap> data) NONIUS_NOEXCEPT : ptr(std::move(data)) {}

    // convenience functions for making data objects
    inline data_ptr make_data(std::string val) {
        return {std::make_shared<DataValue>(std::move(val))};
    }
    inline data_ptr make_data(data_list val) {
        return {std::make_shared<DataList>(std::move(val))};
    }
    inline data_ptr make_data(data_map val) {
        return {std::make_shared<DataMap>(std::move(val))};
    }

    // get a data value from a data map
    // e.g. foo.bar => data["foo"]["bar"]
    data_ptr parse_val(const std::string& key, data_map& data);

    enum TokenType {
        TOKEN_TYPE_NONE,
        TOKEN_TYPE_TEXT,
        TOKEN_TYPE_VAR,
        TOKEN_TYPE_IF,
        TOKEN_TYPE_FOR,
        TOKEN_TYPE_ENDIF,
        TOKEN_TYPE_ENDFOR,
    };

    // Template tokens
    // base class for all token types
    struct Token {
        virtual TokenType gettype() const NONIUS_NOEXCEPT = 0;
        virtual void gettext(std::ostream& stream, data_map& data) const = 0;
        virtual void set_children(token_vector children);
        virtual token_vector& get_children();
    };

    // normal text
    struct TokenText : Token {
        TokenText(std::string text) NONIUS_NOEXCEPT : m_text(std::move(text)) {}
        TokenType gettype() const NONIUS_NOEXCEPT override;
        void gettext(std::ostream& stream, data_map& data) const override;
    private:
        std::string m_text;
    };

    // variable
    struct TokenVar : Token {
        TokenVar(std::string key) NONIUS_NOEXCEPT : m_key(std::move(key)) {}
        TokenType gettype() const NONIUS_NOEXCEPT override;
        void gettext(std::ostream& stream, data_map& data) const override;
    private:
        std::string m_key;
    };

    // for block
    struct TokenFor : Token {
        TokenFor(const std::string& expr);
        TokenType gettype() const NONIUS_NOEXCEPT override;
        void gettext(std::ostream& stream, data_map& data) const override;
        void set_children(token_vector children) override;
        token_vector& get_children() override;
    private:
        std::string m_key;
        std::string m_val;
        token_vector m_children;
    };

    // if block
    struct TokenIf : Token {
        TokenIf(std::string expr) NONIUS_NOEXCEPT : m_expr(std::move(expr)) {}
        TokenType gettype() const NONIUS_NOEXCEPT override;
        void gettext(std::ostream& stream, data_map& data) const override;
        bool is_true(const std::string& expr, data_map& data) const;
        void set_children(token_vector children) override;
        token_vector& get_children() override;
    private:
        std::string m_expr;
        token_vector m_children;
    };

    // end of block
    struct TokenEnd : Token { // end of control block
        TokenEnd(std::string text) NONIUS_NOEXCEPT : m_type(std::move(text)) {}
        TokenType gettype() const NONIUS_NOEXCEPT override;
        void gettext(std::ostream& stream, data_map& data) const override;
    private:
        std::string m_type;
    };

    std::string gettext(const token_ptr& token, data_map& data);

    void parse_tree(token_vector& tokens, token_vector& tree, TokenType until=TOKEN_TYPE_NONE);
    token_vector& tokenize(std::string text, token_vector& tokens);

    // The big daddy. Pass in the template and data,
    // and get out a completed doc.
    void parse(std::ostream& stream, std::string templ_text, data_map& data);
    std::string parse(std::string templ_text, data_map& data);

// *********** Implementation ************

    //////////////////////////////////////////////////////////////////////////
    // Data classes
    //////////////////////////////////////////////////////////////////////////

    // data_map
    inline data_ptr& data_map::operator [](const std::string& key) {
        return data[key];
    }
    inline bool data_map::empty() const NONIUS_NOEXCEPT {
        return data.empty();
    }
    inline bool data_map::has(const std::string& key) const {
        return data.find(key) != data.end();
    }

    // data_ptr
    inline data_ptr& data_ptr::operator =(std::string data) {
        return *this = make_data(std::move(data));
    }
    inline data_ptr& data_ptr::operator =(data_map data) {
        return *this = make_data(std::move(data));
    }

    template<typename T, typename std::enable_if<std::is_constructible<data_ptr, T&&>::value, int>::type>
    inline data_ptr& data_ptr::emplace_back(T&& data) {
        if (!ptr) {
            *this = make_data(data_list());
        }
        data_list& list = ptr->getlist();
        list.emplace_back(std::forward<T>(data));
        return list.back();
    }

    // base data
    inline std::string& Data::getvalue() {
        throw TemplateException("Data item is not a value");
    }
    inline data_list& Data::getlist() {
        throw TemplateException("Data item is not a list");
    }
    inline data_map& Data::getmap() {
        throw TemplateException("Data item is not a dictionary");
    }

    // data value
    inline std::string& DataValue::getvalue() {
        return m_value;
    }
    inline bool DataValue::empty() const NONIUS_NOEXCEPT {
        return m_value.empty();
    }

    // data list
    inline data_list& DataList::getlist() {
        return m_items;
    }
    inline bool DataList::empty() const NONIUS_NOEXCEPT {
        return m_items.empty();
    }

    // data map
    inline data_map& DataMap::getmap() {
        return m_items;
    }
    inline bool DataMap::empty() const NONIUS_NOEXCEPT {
        return m_items.empty();
    }

    //////////////////////////////////////////////////////////////////////////
    // parse_val
    //////////////////////////////////////////////////////////////////////////
    inline data_ptr parse_val(const std::string& key, data_map& data) {
        // quoted string
        if (key[0] == '\"') {
            return make_data(boost::trim_copy_if(key, [](char c) { return c == '\"'; }));
        }

        // check for dotted notation, i.e [foo.bar]
        std::size_t const index = key.find('.');
        if (index == key.npos) {
            if (!data.has(key)) {
                return make_data("{$" + key + '}');
            }
            return data[key];
        }

        std::string const sub_key = key.substr(0, index);
        if (!data.has(sub_key)) {
            return make_data("{$" + key + '}');
        }
        data_ptr& item = data[sub_key];
        return parse_val(key.substr(index + 1), item->getmap());
    }

    //////////////////////////////////////////////////////////////////////////
    // Token classes
    //////////////////////////////////////////////////////////////////////////

    // defaults, overridden by subclasses with children
    inline void Token::set_children(token_vector) {
        throw TemplateException("This token type cannot have children");
    }
    inline token_vector& Token::get_children() {
        throw TemplateException("This token type cannot have children");
    }

    // TokenText
    inline TokenType TokenText::gettype() const NONIUS_NOEXCEPT {
        return TOKEN_TYPE_TEXT;
    }
    inline void TokenText::gettext(std::ostream& stream, data_map&) const {
        stream << m_text;
    }

    // TokenVar
    inline TokenType TokenVar::gettype() const NONIUS_NOEXCEPT {
        return TOKEN_TYPE_VAR;
    }
    inline void TokenVar::gettext(std::ostream& stream, data_map& data) const {
        stream << parse_val(m_key, data)->getvalue();
    }

    // TokenFor
    inline TokenFor::TokenFor(const std::string& expr) {
        std::vector<std::string> elements;
        boost::split(elements, expr, boost::is_space());
        if (elements.size() != 4u) {
            throw TemplateException("Invalid syntax in for statement");
        }
        m_val = std::move(elements[1]);
        m_key = std::move(elements[3]);
    }

    inline TokenType TokenFor::gettype() const NONIUS_NOEXCEPT {
        return TOKEN_TYPE_FOR;
    }

    inline void TokenFor::gettext(std::ostream& stream, data_map& data) const {
        data_ptr value = parse_val(m_key, data);
        const data_list& items = value->getlist();
        for (std::size_t i = 0, i_max = items.size(); i != i_max; ++i) {
            data_map loop;
            loop["index"] = make_data(boost::lexical_cast<std::string>(i + 1));
            loop["index0"] = make_data(boost::lexical_cast<std::string>(i));
            data["loop"] = make_data(std::move(loop));
            data[m_val] = items[i];
            for (std::size_t j = 0, j_max = m_children.size(); j != j_max; ++j) {
                m_children[j]->gettext(stream, data);
            }
        }
    }

    inline void TokenFor::set_children(token_vector children) {
        m_children.assign(std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()));
    }
    inline token_vector& TokenFor::get_children() {
        return m_children;
    }

    // TokenIf
    inline TokenType TokenIf::gettype() const NONIUS_NOEXCEPT {
        return TOKEN_TYPE_IF;
    }

    inline void TokenIf::gettext(std::ostream& stream, data_map& data) const {
        if (is_true(m_expr, data)) {
            for (std::size_t j = 0, j_max = m_children.size(); j != j_max; ++j) {
                m_children[j]->gettext(stream, data);
            }
        }
    }

    inline bool TokenIf::is_true(const std::string& expr, data_map& data) const {
        std::vector<std::string> elements;
        boost::split(elements, expr, boost::is_space());

        if (elements[1] == "not") {
            return parse_val(elements[2], data)->empty();
        }
        if (elements.size() == 2) {
            return !parse_val(elements[1], data)->empty();
        }
        data_ptr lhs = parse_val(elements[1], data);
        data_ptr rhs = parse_val(elements[3], data);
        if (elements[2] == "==") {
            return lhs->getvalue() == rhs->getvalue();
        }
        return lhs->getvalue() != rhs->getvalue();
    }

    inline void TokenIf::set_children(token_vector children) {
        m_children.assign(std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()));
    }
    inline token_vector& TokenIf::get_children() {
        return m_children;
    }

    // TokenEnd
    inline TokenType TokenEnd::gettype() const NONIUS_NOEXCEPT {
        return m_type == "endfor" ? TOKEN_TYPE_ENDFOR : TOKEN_TYPE_ENDIF;
    }

    inline void TokenEnd::gettext(std::ostream&, data_map&) const {
        throw TemplateException("End-of-control statements have no associated text");
    }

    // gettext
    // generic helper for getting text from tokens.
    inline std::string gettext(const token_ptr& token, data_map& data) {
        std::ostringstream stream;
        token->gettext(stream, data);
        return stream.str();
    }

    //////////////////////////////////////////////////////////////////////////
    // parse_tree
    // recursively parses list of tokens into a tree
    //////////////////////////////////////////////////////////////////////////
    inline void parse_tree(token_vector& tokens, token_vector& tree, TokenType const until) {
        while (!tokens.empty()) {
            // 'pops' first item off list
            token_ptr token = std::move(tokens[0]);
            tokens.erase(tokens.begin());

            if (token->gettype() == TOKEN_TYPE_FOR) {
                token_vector children;
                parse_tree(tokens, children, TOKEN_TYPE_ENDFOR);
                token->set_children(std::move(children));
            } else if (token->gettype() == TOKEN_TYPE_IF) {
                token_vector children;
                parse_tree(tokens, children, TOKEN_TYPE_ENDIF);
                token->set_children(std::move(children));
            } else if (token->gettype() == until) {
                break;
            }
            tree.push_back(std::move(token));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // tokenize
    // parses a template into tokens (text, for, if, variable)
    //////////////////////////////////////////////////////////////////////////
    inline token_vector& tokenize(std::string text, token_vector& tokens) {
        while (!text.empty()) {
            std::size_t pos = text.find('{');
            if (pos == text.npos) {
                if (!text.empty()) {
                    tokens.emplace_back(std::make_shared<TokenText>(std::move(text)));
                }
                break;
            }
            if (pos > 0) {
                std::string pre_text = text.substr(0, pos);
                tokens.emplace_back(std::make_shared<TokenText>(std::move(pre_text)));
            }
            if (pos == text.size() - 1) {
                tokens.emplace_back(std::make_shared<TokenText>("{"));
                break;
            }
            text.erase(0, pos + 1);

            // variable
            if (text[0] == '$') {
                pos = text.find('}');
                if (pos != text.npos) {
                    tokens.emplace_back(std::make_shared<TokenVar>(text.substr(1, pos - 1)));
                    text.erase(0, pos + 1);
                }
            } else if (text[0] == '%') { // control statement
                pos = text.find('}');
                if (pos != text.npos) {
                    std::string expression = boost::trim_copy(text.substr(1, pos - 2));
                    text.erase(0, pos + 1);
                    if (boost::starts_with(expression, "for")) {
                        tokens.emplace_back(std::make_shared<TokenFor>(std::move(expression)));
                    } else if (boost::starts_with(expression, "if")) {
                        tokens.emplace_back(std::make_shared<TokenIf>(std::move(expression)));
                    } else {
                        boost::trim(expression);
                        tokens.emplace_back(std::make_shared<TokenEnd>(std::move(expression)));
                    }
                }
            } else {
                tokens.emplace_back(std::make_shared<TokenText>("{"));
            }
        }
        return tokens;
    }

    /************************************************************************
    * parse
    *
    *  1. tokenizes template
    *  2. parses tokens into tree
    *  3. resolves template
    *  4. returns converted text
    ************************************************************************/
    inline std::string parse(std::string templ_text, data_map& data) {
        std::ostringstream stream;
        parse(stream, std::move(templ_text), data);
        return stream.str();
    }
    inline void parse(std::ostream& stream, std::string templ_text, data_map& data) {
        token_vector tokens;
        tokenize(std::move(templ_text), tokens);
        token_vector tree;
        parse_tree(tokens, tree);

        for (std::size_t i = 0, i_max = tree.size(); i != i_max; ++i) {
            // Recursively calls gettext on each node in the tree.
            // gettext returns the appropriate text for that node.
            // for text, itself;
            // for variable, substitution;
            // for control statement, recursively gets kids
            tree[i]->gettext(stream, data);
        }
    }
}

#endif // CPPTEMPL_H
