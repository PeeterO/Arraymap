/*
 * Copyright (c) 2020 Peeter Org 
 * 
 * Permission is hereby granted, free of charge, 
 * to any person obtaining a copy of this software and 
 * associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice 
 * shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _ARRAYMAP_H_
#define _ARRAYMAP_H_

#include <functional>
#include <cstring>
#include <stdexcept>
#include <cstdint>


/*
 * This code attempts to mimic the external behaviour of std::map, therefore is left uncommented intentionally.
 * To find out what public methods are supposed to do what, see documentation of std::map.
 *
 * The idea is that you should finish you solution with std::map first and then (if std::map's performance isn't enough)
 * change the declaration of your map (and probably few more nuances) in your code to use this container instead
 *
 * If you find something that that doesn't behave like std::map, file a bug report (or make a pull request).
 *
 * Following behaviour is different from std::map and cannot be modified due to different working priciple:
 *
 *        * ordering element used instead of compare 
 *	  *
 *        * allocator is for value type only instead of std::pair<key, value>
 *	  *
 *        * certain types (like node_type) and methods are missing
 *	  *
 *        * iterator->second is an std::reference_wrapper of mapped type rather than just reference of mapped type.
 *
 * 		* Therefore range-based loops need to be value-based:
 *		* 	for(auto [key, value] : theMap ){...}
 *		*
 *		* rather than reference-based;
 *		* 	for(auto & [key, value] : theMap ){...}
 */

namespace arraymap{

	template <class T>
		class ordering_none{
			public:
				static T apply(const T &value){ return value; };
				static T restore(const T &value){ return value; };
		};

	template <class T>
		class ordering_default{
			public:
				static T apply(const T &value)
				{ 
					if constexpr(std::is_same<T, std::int64_t>::value)
					{
						return  value^((std::int64_t)1<<63);
					}
					else if constexpr(std::is_same<T, std::int32_t>::value)
					{
						return value^(1<<31);
					}
					else if constexpr(std::is_same<T, std::int16_t>::value)
					{
						return value^(1<<15);
					}
					else if constexpr(std::is_same<T, std::int8_t>::value)
					{
						return value^(1<<7);
					}
					else if constexpr(std::is_same<T, float>::value)
					{
						float nonconstValue = value; 
						std::uint32_t* pf = (std::uint32_t*)(&nonconstValue);
						*pf^=(0x80000000);
						*pf^= ( (0x7FFFFFFF)*( !( *pf & 0x80000000 ) ) ); 
						return *( (float*)(pf) ); 
					}
					else if constexpr(std::is_same<T, double>::value)
					{
						double nonconstValue = value; 
						std::uint64_t* pf = (std::uint64_t*)(&nonconstValue);
						*pf^=((std::uint64_t)0x8000000000000000);
						*pf^= ( ((std::uint64_t)0x7FFFFFFFFFFFFFFF)*( !( *pf & (std::uint64_t)0x8000000000000000 ) ) ); 
						return *( (double*)(pf) ); 
					}
					return value; 
				};
				static T restore(const T &value)
				{

					if constexpr(std::is_same<T, std::int64_t>::value)
					{
						return value^((std::int64_t)1<<63);
					}
					else if constexpr(std::is_same<T, std::int32_t>::value)
					{
						return value^(1<<31);
					}
					else if constexpr(std::is_same<T, std::int16_t>::value)
					{
						return value^(1<<15);
					}
					else if constexpr(std::is_same<T, std::int8_t>::value)
					{
						return value^(1<<7);
					}
					else if constexpr(std::is_same<T, float>::value)
					{
						float nonconstValue = value;
						std::uint32_t* pf = (std::uint32_t*)(&nonconstValue);
						*pf^=(0x80000000);
						*pf^= ( (0x7FFFFFFF)*( !!( *pf & 0x80000000 ) ) );
						return *( (float*)(pf) ); 
					}
					else if constexpr(std::is_same<T, double>::value)
					{
						double nonconstValue = value; 
						std::uint64_t* pf = (std::uint64_t*)(&nonconstValue);
						*pf^=((std::uint64_t)0x8000000000000000);
						*pf^= ( ((std::uint64_t)0x7FFFFFFFFFFFFFFF)*( !!( *pf & (std::uint64_t)0x8000000000000000 ) ) ); 
						return *( (double*)(pf) ); 
					}
					return value;
				};
		};

	template<
		class _Key,
		class _Tp,
		class Ordering = ordering_default<_Key>,
		class Allocator = std::allocator<_Tp>
	      >
	      class arraymap{
		      public:
			      typedef _Key    key_type;
			      typedef _Tp     mapped_type;
			      typedef Ordering ordering_elem;
			      typedef std::pair<key_type, mapped_type> value_type;
			      typedef std::pair<key_type, std::reference_wrapper<mapped_type>> reference;
			      typedef std::size_t size_type;
			      typedef Allocator allocator_type;
			      class iterator;
			      class reverse_iterator;
			      class const_iterator;
			      class const_reverse_iterator;
		      private:
			      static_assert(std::is_trivially_copyable<key_type>::value, "Arraymap requires key type to be trivially copyable! (std::is_trivially_copyable)");
			      typedef union ptr_s{
				      mapped_type *val;
				      ptr_s *next;
				      bool operator==(const ptr_s &p)
				      { return next == p.next; };
				      bool operator!=(const ptr_s &p)
				      { return next != p.next; };
			      }ptr_t;
			      static Allocator alloc;
		      private:
			      static inline ptr_t empty_node[0x10];
			      ptr_t root;
			      iterator end_it;
			      const_iterator cend_it;
			      reverse_iterator rend_it;
			      const_reverse_iterator crend_it;
			      size_type element_count;
			      static constexpr unsigned short MAX_DEPTH = sizeof(key_type) * 2;
		      public:
			      arraymap(void):
				      end_it(&this->root),
				      cend_it(&this->root),
				      rend_it(&this->root),
				      crend_it(&this->root),
				      element_count(0)
			      {
				      for(int i = 0; i < 0x10; i++)
					      empty_node[i].next = empty_node; //TODO figure out a way to get rid of this initialization in constructor and initialize empty_node only once

				      root.next = empty_node;
			      }
			      arraymap(std::initializer_list<value_type> ilist):
				      arraymap()
			      {
				      insert(ilist);
			      }
			      arraymap(const arraymap &other):
				      arraymap()
			      {
				      for(auto it = other.cbegin(); it != other.cend(); it++)
				      {
					      insert(*it);
				      }
			      }
			      arraymap(arraymap &&other):
				      end_it(other.end_it),
				      cend_it(other.cend_it),
				      rend_it(other.rend_it),
				      crend_it(other.rend_it),
				      element_count(other.element_count)
			      {
				      root = other.root;
				      element_count = other.element_count;
				      other.root = empty_node;
				      other.element_count = 0;
			      }
			      arraymap &operator=(const arraymap& other)
			      {
				      clear();
				      for(auto it = other.cbegin(); it != other.cend(); it++)
				      {
					      insert(*it);
				      }
				      return *this;
			      }
			      arraymap &operator=(const arraymap&& other) noexcept
			      {
				      clear();
				      root = other.root;
				      element_count = other.element_count;
				      other.root = empty_node;
				      other.element_count = 0;
				      return *this;
			      }
			      arraymap &operator=(std::initializer_list<value_type> ilist) noexcept
			      {
				      clear();
				      insert(ilist);
				      return *this;
			      }
			      ~arraymap(void)
			      {
				      free_node_tree(&root, MAX_DEPTH);
			      }
		      private: 
			      class iterator_base{
				      protected:
					      ptr_t *stack[MAX_DEPTH];
					      short depth;
					      unsigned char key_bytes[sizeof(key_type) + 1];
					      mapped_type empty_element;
					      reference currentValue;

					      iterator_base(ptr_t *root):
						      currentValue(*(key_type*)iterator_base::key_bytes, empty_element) 
					      {
						      stack[MAX_DEPTH - 1] = root;
					      }
					      iterator_base(ptr_t *root,const key_type &key):
						      iterator_base(root)
					      {
						      std::memcpy(key_bytes, &key, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 0;
						      fill_ptr_stack();
						      for(depth = MAX_DEPTH - 1 ; depth > 0 && *stack[depth] != *empty_node ; depth--);

						      if(target_valid())
						      {
							      currentValue.first = Ordering::restore(key);
							      currentValue.second = *stack[0]->next[tetradeValue(key_bytes, 0)].val;
						      }
					      }
					      bool target_valid()
					      {
						      return (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes, 0)] != *empty_node );
					      }
					      void fill_ptr_stack()
					      {
						      ptr_t *current_Node = stack[MAX_DEPTH-1];
						      unsigned int tetradeNr =  MAX_DEPTH - 2;
						      do{
							      current_Node = ( current_Node->next + tetradeValue(key_bytes, tetradeNr+1));
							      stack[tetradeNr] = current_Node;
						      }while(tetradeNr-- > 0);
					      }
					      static int incrQuartet(unsigned char *bytes, unsigned int quartetNr)
					      {
						      if(quartetNr > MAX_DEPTH) return 0;
						      bytes[(quartetNr) >> 1] += 1 << ( ( quartetNr) & 1 ) * 4;

						      if( ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4 ) ) == 0 )
							      return !(quartetNr & 1) + 1 + incrQuartet(bytes, quartetNr + 1 + !(quartetNr & 1));

						      return ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4)  & 0xF )  == 0;
					      }
					      static int decrQuartet(unsigned char *bytes, unsigned int quartetNr)
					      {
						      if(quartetNr > MAX_DEPTH) return 0;
						      bytes[(quartetNr) >> 1] -= 1 << ( ( quartetNr) & 1 ) * 4;

						      if( ( (bytes[( quartetNr >> 1 )] >> ( ( quartetNr & 1 ) * 4 )) ) == ( 0xFF >> ( (quartetNr & 1 ) * 4 ) ))
							      return !(quartetNr & 1) + 1 + decrQuartet(bytes, quartetNr + 1 + !(quartetNr & 1 ));

						      return ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4)  & 0xF )  == 0xF;
					      }
					      void increment()
					      {
						      while(depth < MAX_DEPTH){
							      if((depth != 0) && stack[depth]->next[tetradeValue(key_bytes, depth)] != *empty_node)
							      {
								      stack[depth-1] = stack[depth]->next + tetradeValue(key_bytes, depth);
								      depth--;
							      }
							      else 
								      depth += incrQuartet(key_bytes, depth);

							      if( (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes,0)] != *empty_node ) )
							      {
								      currentValue.first = Ordering::restore( (key_type) *( (key_type*) key_bytes ) ); 
								      currentValue.second = *stack[0]->next[tetradeValue(key_bytes,0)].val;
								      return;
							      }
						      }

						      if(key_bytes[sizeof(key_type)] == 0xFF)
						      {
							      key_bytes[sizeof(key_type)] = 0x00;
							      memset(key_bytes, 0x00, sizeof(key_type));
							      depth = MAX_DEPTH - 1;
							      increment();
							      return;
						      }
						      memset(key_bytes, 0, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 1;
						      return;
					      }
					      void decrement()
					      {
						      while(depth < MAX_DEPTH){
							      if((depth != 0) && stack[depth]->next[tetradeValue(key_bytes, depth)] != *empty_node)
							      {
								      stack[depth-1] = stack[depth]->next + tetradeValue(key_bytes, depth);
								      depth--;
							      }
							      else 
								      depth += decrQuartet(key_bytes, depth);

							      if( (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes,0)] != *empty_node ) )
							      {
								      currentValue.first = Ordering::restore( (key_type) *( (key_type*) key_bytes ) ); 
								      currentValue.second = *stack[0]->next[tetradeValue(key_bytes,0)].val;
								      return;
							      }
						      }

						      if(key_bytes[sizeof(key_type)] == 0x01)
						      {
							      key_bytes[sizeof(key_type)] = 0x00;
							      memset(key_bytes, 0xFF, sizeof(key_type));
							      depth = MAX_DEPTH - 1;
							      decrement();
							      return;
						      }
						      memset(key_bytes, 0, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 0xFF;
						      return;
					      }
					      bool erase_elem()
					      {
						      if(depth == 0)
						      {
							      if(( stack[0]->next[tetradeValue(key_bytes,0)] != *empty_node ))
							      {
								      std::destroy_at(stack[0]->next[tetradeValue(key_bytes,0)].val);
								      alloc.deallocate(stack[0]->next[tetradeValue(key_bytes,0)].val, 1);
								      stack[0]->next[tetradeValue(key_bytes,0)].next = empty_node->next;
							      }
							      else return false;

							      for(; depth < MAX_DEPTH; depth++)
							      {
								      if(is_node_empty(*stack[depth]))
								      {
									      delete[] stack[depth]->next;
									      stack[depth]->next = empty_node->next;
								      }
								      else break;
							      }
							      return true;
						      }
						      return false;
					      }
				      public:
					      reference *operator->() 
					      {
						      return &currentValue;
					      }
					      reference operator*() 
					      {
						      return currentValue;
					      }
					      bool operator==(const iterator_base &p) const
					      {
						      return std::memcmp(key_bytes, p.key_bytes, sizeof(key_type)+1) == 0;
					      }
					      bool operator!=(const iterator_base &p) const
					      {
						      return std::memcmp(key_bytes, p.key_bytes, sizeof(key_type)+1) != 0;
					      }
			      };
			      class const_iterator_base{
				      protected:
					      const ptr_t *stack[MAX_DEPTH];
					      short depth;
					      unsigned char key_bytes[sizeof(key_type) + 1];
					      mapped_type empty_element;
					      reference currentValue;

					      const_iterator_base(const ptr_t *root):
						      currentValue(*(key_type*)const_iterator_base::key_bytes, empty_element) 
					      {
						      stack[MAX_DEPTH - 1] = root;
					      }
					      const_iterator_base(const ptr_t *root, key_type &key):
						      const_iterator_base(root)
					      {
						      std::memcpy(key_bytes, &key, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 0;
						      fill_ptr_stack();
						      for(depth = MAX_DEPTH - 1 ; depth > 0 && *stack[depth] != *empty_node ; depth--);

						      if(target_valid())
						      {
							      currentValue.first = Ordering::restore(key);
							      currentValue.second = *stack[0]->next[tetradeValue(key_bytes, 0)].val;
						      }
					      }
					      bool target_valid()
					      {
						      return (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes, 0)] != *empty_node );
					      }
					      void fill_ptr_stack()
					      {
						      const ptr_t *current_Node = stack[MAX_DEPTH-1];
						      unsigned int tetradeNr =  MAX_DEPTH - 2;
						      do{
							      current_Node = ( current_Node->next + tetradeValue(key_bytes, tetradeNr+1));
							      stack[tetradeNr] = current_Node;
						      }while(tetradeNr-- > 0);
					      }
					      static int incrQuartet(unsigned char *bytes, unsigned int quartetNr)
					      {
						      if(quartetNr > MAX_DEPTH) return 0;
						      bytes[(quartetNr) >> 1] += 1 << ( ( quartetNr) & 1 ) * 4;

						      if( ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4 ) ) == 0 )
							      return !(quartetNr & 1) + 1 + incrQuartet(bytes, quartetNr + 1 + !(quartetNr & 1));

						      return ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4)  & 0xF )  == 0;
					      }
					      static int decrQuartet(unsigned char *bytes, unsigned int quartetNr)
					      {
						      if(quartetNr > MAX_DEPTH) return 0;
						      bytes[(quartetNr) >> 1] -= 1 << ( ( quartetNr) & 1 ) * 4;

						      if( ( (bytes[( quartetNr >> 1 )] >> ( ( quartetNr & 1 ) * 4 )) ) == ( 0xFF >> ( (quartetNr & 1 ) * 4 ) ))
							      return !(quartetNr & 1) + 1 + decrQuartet(bytes, quartetNr + 1 + !(quartetNr & 1 ));

						      return ( (bytes[( quartetNr >> 1 )] >> ( quartetNr & 1 ) * 4)  & 0xF )  == 0xF;
					      }
					      void increment()
					      {
						      while(depth < MAX_DEPTH)
						      {
							      if((depth != 0) && stack[depth]->next[tetradeValue(key_bytes, depth)] != *empty_node)
							      {
								      stack[depth-1] = stack[depth]->next + tetradeValue(key_bytes, depth);
								      depth--;
							      }
							      else 
								      depth += incrQuartet(key_bytes, depth);

							      if( (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes,0)] != *empty_node ) )
							      {
								      currentValue.first = Ordering::restore( (key_type) *( (key_type*) key_bytes ) ); 
								      currentValue.second = *stack[0]->next[tetradeValue(key_bytes,0)].val;
								      return;
							      }
						      }

						      if(key_bytes[sizeof(key_type)] == 0xFF)
						      {
							      key_bytes[sizeof(key_type)] = 0x00;
							      memset(key_bytes, 0x00, sizeof(key_type));
							      depth = MAX_DEPTH - 1;
							      increment();
							      return;
						      }
						      memset(key_bytes, 0, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 1;
						      return;
					      }
					      void decrement()
					      {
						      while(depth < MAX_DEPTH)
						      {
							      if((depth != 0) && stack[depth]->next[tetradeValue(key_bytes, depth)] != *empty_node)
							      {
								      stack[depth-1] = stack[depth]->next + tetradeValue(key_bytes, depth);
								      depth--;
							      }
							      else 
								      depth += decrQuartet(key_bytes, depth);

							      if( (depth == 0) && ( stack[0]->next[tetradeValue(key_bytes,0)] != *empty_node ) )
							      {
								      currentValue.first = Ordering::restore( (key_type) *( (key_type*) key_bytes ) ); 
								      currentValue.second = *stack[0]->next[tetradeValue(key_bytes,0)].val;
								      return;
							      }
						      }

						      if(key_bytes[sizeof(key_type)] == 0x01)
						      {
							      key_bytes[sizeof(key_type)] = 0x00;
							      memset(key_bytes, 0xFF, sizeof(key_type));
							      depth = MAX_DEPTH - 1;
							      decrement();
							      return;
						      }
						      memset(key_bytes, 0, sizeof(key_type));
						      key_bytes[sizeof(key_type)] = 0xFF;
						      return;
					      }
				      public:
					      reference *operator->() 
					      {
						      return &currentValue;
					      }
					      reference operator*() 
					      {
						      return currentValue;
					      }
					      bool operator==(const const_iterator_base &p) const
					      {
						      return std::memcmp(key_bytes, p.key_bytes, sizeof(key_type)+1) == 0;
					      }
					      bool operator !=(const const_iterator_base &p) const
					      {
						      return std::memcmp(key_bytes, p.key_bytes, sizeof(key_type)+1) != 0;
					      }
			      };
		      public:
			      class iterator : public iterator_base{
				      private:
					      friend class arraymap;
				      iterator(ptr_t *root, key_type key, bool findNext):
					      iterator_base(root, key)
				      {
					      if(!iterator_base::target_valid())
					      {
						      if(findNext)
						      {
							      operator++();
						      }else
						      {
							      memset(iterator_base::key_bytes, 0, sizeof(key_type));
							      iterator_base::key_bytes[sizeof(key_type)] = 1;
							      iterator_base::depth = MAX_DEPTH;
						      }
					      }
				      }
				      iterator(ptr_t *root):
					      iterator_base(root)
				      {
					      memset(iterator_base::key_bytes, 0, sizeof(key_type));
					      iterator_base::key_bytes[sizeof(key_type)] = 1;
					      iterator_base::depth = MAX_DEPTH;
				      }
				      public:
					      iterator operator--(int)
					      {
						      iterator tmp(*this);
						      operator--();
						      return tmp;
					      }
					      iterator& operator--()
					      {
						      iterator_base::decrement();
						      return *this;
					      }
					      iterator operator++(int)
					      {
						      iterator tmp(*this);
						      operator++();
						      return tmp; 
					      }
					      iterator& operator++()
					      {
						      iterator_base::increment();
						      return *this;
					      }
			      };
			      class reverse_iterator : public iterator_base{
				      private:
					      friend class arraymap;
				      reverse_iterator(ptr_t *root, key_type key, bool findNext):
					      iterator_base(root, key)
				      {
					      if(!iterator_base::target_valid()){
						      if(findNext)
						      {
							      operator++();
						      }else
						      {

							      memset(iterator_base::key_bytes, 0, sizeof(key_type));
							      iterator_base::key_bytes[sizeof(key_type)] = 0xFF;
							      iterator_base::depth = MAX_DEPTH;
						      }
					      }
				      }
				      reverse_iterator(ptr_t *root):
					      iterator_base(root)
				      {
					      memset(iterator_base::key_bytes, 0, sizeof(key_type));
					      iterator_base::key_bytes[sizeof(key_type)] = 0xFF;
					      iterator_base::depth = MAX_DEPTH;
				      }
				      public:
					      reverse_iterator operator--(int)
					      {
						      reverse_iterator tmp(*this);
						      operator--();
						      return tmp;
					      }
					      reverse_iterator& operator--()
					      {
						      iterator_base::increment();
						      return *this;
					      }
					      reverse_iterator operator++(int)
					      {
						      reverse_iterator tmp(*this);
						      operator++();
						      return tmp; 
					      }
					      reverse_iterator& operator++()
					      {
						      iterator_base::decrement();
						      return *this;
					      }
			      };
			      class const_iterator : public const_iterator_base{
				      private:
					      friend class arraymap;
				      const_iterator(const ptr_t *root, key_type key, bool findNext):
					      const_iterator_base(root, key)
				      {
					      if(!const_iterator_base::target_valid()){
						      if(findNext)
						      {
							      operator++();
						      }else
						      {
							      memset(const_iterator_base::key_bytes, 0, sizeof(key_type));
							      const_iterator_base::key_bytes[sizeof(key_type)] = 1;
							      const_iterator_base::depth = MAX_DEPTH;
						      }
					      }
				      }
				      const_iterator(ptr_t *root):
					      const_iterator_base(root)
				      {
					      memset(const_iterator_base::key_bytes, 0, sizeof(key_type));
					      const_iterator_base::key_bytes[sizeof(key_type)] = 1;
					      const_iterator_base::depth = MAX_DEPTH;
				      }
				      public:
					      const_iterator operator--(int)
					      {
						      const_iterator tmp(*this);
						      operator--();
						      return tmp;
					      }
					      const_iterator& operator--()
					      {
						      const_iterator_base::decrement();
						      return *this;
					      }
					      const_iterator operator++(int)
					      {
						      const_iterator tmp(*this);
						      operator++();
						      return tmp; 
					      }
					      const_iterator& operator++()
					      {
						      const_iterator_base::increment();
						      return *this;
					      }
			      };
			      class const_reverse_iterator : public const_iterator_base{
				      private:
					      friend class arraymap;
				      const_reverse_iterator(const ptr_t *root, key_type key, bool findNext):
					      const_iterator_base(root, key)
				      {
					      if(!const_iterator_base::target_valid()){
						      if(findNext)
						      {
							      operator++();
						      }else
						      {
							      memset(const_iterator_base::key_bytes, 0, sizeof(key_type));
							      const_iterator_base::key_bytes[sizeof(key_type)] = 0xFF;
							      const_iterator_base::depth = MAX_DEPTH;
						      }
					      }
				      }
				      const_reverse_iterator(ptr_t *root):
					      const_iterator_base(root)
				      {
					      memset(const_iterator_base::key_bytes, 0, sizeof(key_type));
					      const_iterator_base::key_bytes[sizeof(key_type)] = 0xFF;
					      const_iterator_base::depth = MAX_DEPTH;
				      }
				      public:
					      const_reverse_iterator operator--(int)
					      {
						      const_reverse_iterator tmp(*this);
						      operator--();
						      return tmp;
					      }
					      const_reverse_iterator& operator--()
					      {
						      const_iterator_base::increment();
						      return *this;
					      }
					      const_reverse_iterator operator++(int)
					      {
						      const_reverse_iterator tmp(*this);
						      operator++();
						      return tmp; 
					      }
					      const_reverse_iterator& operator++()
					      {
						      const_iterator_base::decrement();
						      return *this;
					      }
			      };
			      mapped_type& operator[](const key_type& __k)
			      {
				      ptr_t result = element_fast_find(Ordering::apply( __k ));

				      if(result == *empty_node)
				      {
					      result = element_fast_add(Ordering::apply( __k ));
					      element_count++;
				      }

				      return *result.val;
			      }
			      mapped_type& at(const key_type& __k) const
			      {
				      ptr_t result = element_fast_find(Ordering::apply( __k ));

				      if(result == *empty_node)
					      throw std::out_of_range("arraymap::at");

				      return *result->val;
			      }
			      bool empty() const noexcept
			      {
				      return element_count == 0;
			      }
			      void clear() noexcept
			      {
				      free_node_tree(&root, MAX_DEPTH);
				      root.next = empty_node;
				      element_count = 0;
			      }
			      iterator begin() noexcept
			      {
				      unsigned char bytes[sizeof(key_type)];
				      memset(bytes, 0, sizeof(key_type));
				      return iterator(&this->root, *( (key_type*) bytes), true);
			      }
			      const_iterator cbegin() const noexcept
			      {
				      unsigned char bytes[sizeof(key_type)];
				      memset(bytes, 0, sizeof(key_type));
				      return const_iterator(&this->root, *( (key_type*) bytes), true);
			      }
			      const iterator &end() const noexcept
			      {
				      return end_it;
			      }
			      const const_iterator &cend() const noexcept
			      {
				      return cend_it;
			      }
			      reverse_iterator rbegin() noexcept
			      {
				      unsigned char bytes[sizeof(key_type)];
				      memset(bytes, 0xFF, sizeof(key_type));
				      return reverse_iterator(&this->root, *( (key_type*) bytes), true);
			      }
			      reverse_iterator rend() noexcept
			      {
				      return rend_it;
			      }
			      const_reverse_iterator crbegin() noexcept
			      {
				      unsigned char bytes[sizeof(key_type)];
				      memset(bytes, 0xFF, sizeof(key_type));
				      return const_reverse_iterator(&this->root, *( (key_type*) bytes), true);
			      }
			      const const_reverse_iterator &crend() noexcept
			      {
				      return crend_it;
			      }
			      iterator find(const key_type &key)
			      {
				      return iterator(&root, Ordering::apply(key), false);
			      }
			      iterator lower_bound(const key_type &key)
			      {
				      return iterator(&root, Ordering::apply(key), true);
			      }
			      iterator upper_bound(const key_type &key)
			      {
				      iterator it(&root, Ordering::apply(key), false);
				      if(it == end_it)
					      return iterator(&root, Ordering::apply(key), true);
				      else
					      return ++it;
			      }
			      size_type erase(const key_type& key)
			      {
				      iterator it(&root, Ordering::apply(key), false);
				      if(it != end_it)
				      {
					      it.erase_elem();
					      element_count--;
					      return 1;
				      }
				      else
					      return 0;
			      }
			      iterator erase(iterator pos)
			      {
				      pos.erase_elem();
				      element_count--;
				      return ++pos;
			      }
			      void erase(iterator first, iterator last)
			      {
				      for(;first != last; first++) 
				      {
					      first.erase_elem();
					      element_count--;
				      }
			      }
			      size_type size()
			      {
				      return element_count;
			      }
			      std::pair<iterator,bool> insert(value_type &&value)
			      {
				      bool added = false;
				      ptr_t result = element_fast_find(Ordering::apply(value.first));

				      if(result == *empty_node)
				      {
					      result = element_fast_add(Ordering::apply(value.first), std::move( value.second ));
					      element_count++;
					      added = true;
				      }
				      return std::make_pair(iterator(&root, Ordering::apply(value.first),false), added);
			      }
			      std::pair<iterator,bool> insert(const value_type &value)
			      {
				      bool added = false;
				      ptr_t result = element_fast_find(Ordering::apply(value.first));

				      if(result == *empty_node)
				      {
					      result = element_fast_add(Ordering::apply(value.first),  value.second );
					      element_count++;
					      added = true;
				      }
				      return std::make_pair(iterator(&root, Ordering::apply(value.first),false), added);
			      }
			      void insert(std::initializer_list<value_type> ilist)
			      {
				      for(value_type val : ilist)
					      insert(val);
			      }
			      void insert(const arraymap &other)
			      {
				      for(auto it = other.cbegin(); it != other.cend(); it++)
					      insert(*it);
			      }
			      template< class... Args >
			      std::pair<iterator,bool> emplace( Args&&... args )
			      {
				      return insert(value_type(std::forward<Args>(args)...));
			      }
			      template < class... Args >
			      std::pair<iterator,bool> try_emplace(key_type &&k, Args&&... args)
			      {     
				      bool added = false;
				      ptr_t result = element_fast_find(Ordering::apply(k));

				      if(result == *empty_node)
				      {
					      result = element_fast_add(Ordering::apply(k), std::forward<Args>(args)...);
					      element_count++;
					      added = true;
				      }
				      return std::make_pair(iterator(&root, Ordering::apply(k),false), added);
			      }
			      bool contains(const key_type& __k)
			      {
				      ptr_t result = element_fast_find(Ordering::apply(__k));
				      return result != *empty_node;
			      }
		      private:
			      ptr_t node_add_empty_node(void)
			      {
				      ptr_t ret;
				      ret.next = new ptr_t[0x10];

				      for(int i = 0; i < 0x10; i++)
					      ret.next[i].next = empty_node;

				      return ret;
			      }
			      static void free_node_tree(ptr_t *ptr, unsigned int depth)
			      {
				      if(*ptr == *empty_node) return;

				      if(depth == 0) 
				      {
					      std::destroy_at(ptr->val);
					      alloc.deallocate(ptr->val, 1);
					      return;
				      }

				      for(int i = 0; i < 0x10; i++)
					      free_node_tree(ptr->next + i, depth - 1);

				      delete[] ptr->next;
			      }
			      static bool is_node_empty(ptr_t node) 
			      {
				      for(int i = 0; i < 0x10; i++)
					      if(node.next[i] != *empty_node) return false;
				      return true;
			      }
			      inline ptr_t element_fast_find(const key_type key) const
			      {
				      unsigned char *key_bytes = (unsigned char*)&key;
				      const ptr_t *current_Node = &root;
				      unsigned int tetradeNr =  MAX_DEPTH - 1;
				      do{
					      current_Node = ( current_Node->next + tetradeValue(key_bytes, tetradeNr));
				      }while(tetradeNr-- > 0);
				      return *current_Node;
			      }
			      ptr_t element_fast_add(const key_type key)
			      {
				      unsigned char *key_bytes = (unsigned char*)&key;
				      ptr_t *current_Node = &root;
				      unsigned int tetradeNr =  MAX_DEPTH - 1;
				      do{
					      if(*current_Node == *empty_node)
						      *current_Node = node_add_empty_node();
					      current_Node =( current_Node->next + tetradeValue(key_bytes, tetradeNr));
				      }while(tetradeNr-- > 0);

				      current_Node->val = alloc.allocate(1);
				      std::construct_at(current_Node->val);
				      return *current_Node;
			      }
			      ptr_t element_fast_add(const key_type key, mapped_type value)
			      {
				      unsigned char *key_bytes = (unsigned char*)&key;
				      ptr_t *current_Node = &root;
				      unsigned int tetradeNr =  MAX_DEPTH - 1;
				      do{
					      if(*current_Node == *empty_node)
						      *current_Node = node_add_empty_node();
					      current_Node =( current_Node->next + tetradeValue(key_bytes, tetradeNr));
				      }while(tetradeNr-- > 0);

				      current_Node->val = alloc.allocate(1);
				      std::construct_at(current_Node->val, std::move(value));
				      return *current_Node;
			      }
			      template< class... Args >
			      ptr_t element_fast_add(const key_type key,Args&&... args)
			      {
				      unsigned char *key_bytes = (unsigned char*)&key;
				      ptr_t *current_Node = &root;
				      unsigned int tetradeNr =  MAX_DEPTH - 1;
				      do{
					      if(*current_Node == *empty_node)
						      *current_Node = node_add_empty_node();
					      current_Node =( current_Node->next + tetradeValue(key_bytes, tetradeNr));
				      }while(tetradeNr-- > 0);

				      current_Node->val = alloc.allocate(1);
				      std::construct_at(current_Node->val, std::forward<Args>(args)...);
				      return *current_Node;
			      }
			      static inline unsigned char tetradeValue(const unsigned char *bytes, unsigned int Nr)
			      {
				      static const unsigned char bitShift[2] = { 0, 4 };
				      return ( bytes[Nr >> 1] >> bitShift[Nr & 1] ) & 0xF;
			      }
	      };
}

#endif
