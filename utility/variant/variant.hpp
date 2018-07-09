/**
 *
 * File varaint.hpp
 * 
 * use it like union
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-20
 */

#ifndef __ydk_utility_variant_variant_hpp__
#define __ydk_utility_variant_variant_hpp__

#include <type_traits>
#include <typeindex>

namespace utility
{
    template<size_t arg, size_t ...rest>
    struct max_integer;

    template<size_t arg>
    struct max_integer<arg> : std::integral_constant < size_t, arg > {};

    template<size_t arg1, size_t arg2, size_t ...rest>
    struct max_integer<arg1, arg2, rest...> : std::integral_constant < size_t,
        arg1 >= arg2 ?
        max_integer<arg1, rest...>::value :
        max_integer<arg2, rest...>::value > {};

    template<typename...args>
    struct max_algin : std::integral_constant < size_t, max_integer<std::alignment_of<args>::value...>::value > {};

    template<typename type, typename...list>
    struct contains_type;

    template<typename type>
    struct contains_type<type> : std::false_type{};

    template<typename type, typename head, typename... rest>
    struct contains_type<type, head, rest...>
        : std::conditional< std::is_same<type, head>::value, std::true_type, contains_type<type, rest...>>::type{};

    template<typename...args>
    struct variant_helper;

    template<typename type, typename... args>
    struct variant_helper < type, args... >
    {
        inline static void destory(std::type_index id, void* data)
        {
            if (id == std::type_index(typeid(type)))
                reinterpret_cast<type*>(data)->~type();
            else
                variant_helper<args...>::destory(id, data);
        }

        inline static void move(std::type_index old_t, void* old_v, void* new_v)
        {
            if (old_t == std::type_index(typeid(type)))
                new (new_v)type(std::move(*reinterpret_cast<type*>(old_v)));
            else
                variant_helper<args...>::move(old_t, old_v, new_v);
        }

        inline static void copy(std::type_index old_t, const void* old_v, void* new_v)
        {
            if (old_t == std::type_index(typeid(type)))
                new (new_v)type(*reinterpret_cast<const type*>(old_v));
            else
                variant_helper<args...>::copy(old_t, old_v, new_v);
        }
    };

    template<>
    struct variant_helper < >
    {
        inline static void destory(std::type_index id, void* data){}
        inline static void move(std::type_index old_t, void* old_v, void* new_v){}
        inline static void copy(std::type_index old_t, const void* old_v, void* new_v){}
    };

    template<typename ...types>
    class variant
    {
    public:
        enum
        {
            data_size = max_integer<sizeof(types)...>::value,
            algin_size = max_algin<types...>::value,
        };

    protected:

        typedef variant_helper<types...> helper_t;

        using data_t = typename std::aligned_storage<data_size, algin_size>::type;

    protected:
        data_t              data_;
        std::type_index     type_index_;

    public:
        variant(void)
            :type_index_(typeid(void))
        {
        }

        ~variant()
        {
            helper_t::destory(type_index_, &data_);
        }

        variant(variant<types...>&& old)
            :type_index_(old.type_index_)
        {
            helper_t::move(old.type_index_, &old.data_, &data_);
        }

        variant(const variant<types...>& old)
            :type_index_(old.type_index_)
        {
            helper_t::copy(old.type_index_, &old.data_, &data_);
        }

        variant& operator=(const variant& old)
        {
            type_index_ = old.type_index_;
            helper_t::copy(old.type_index_, &old.data_, &data_);

            return *this;
        }

        variant& operator=(variant&& old)
        {
            type_index_ = old.type_index_;
            helper_t::move(old.type_index_, &old.data_, &data_);

            return *this;
        }

        template<class type,
        class = typename std::enable_if<contains_type< typename std::remove_reference<type>::type, types...>::value>::type>
            variant(type&& value)
            : type_index_(typeid(void))
        {
            helper_t::destory(type_index_, &data_);
            typedef typename std::remove_reference<type>::type u_type;
            new(&data_) u_type(std::forward<type>(value));
            type_index_ = std::type_index(typeid(type));
        }

        template<class type>
        bool is() const
        {
            return type_index_ == std::type_index(typeid(type));
        }

        bool empty() const
        {
            return type_index_ == std::type_index(typeid(void));
        }

        std::type_index type() const
        {
            return type_index_;
        }

        template<typename type>
        typename std::decay<type>::type& get()
        {
            using u_type = std::decay<type>::type;
            if (!is<u_type>())
            {
                throw std::bad_cast();
            }

            return *((u_type*)&data_);
        }
    };
}

#endif