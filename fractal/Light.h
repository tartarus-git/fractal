#pragma once

#include "nmath/vectors/Vector3f.h"

/*
* 
* SOME NOTES ABOUT ALIGNMENT:
*	- You already know that alignment helps to make memory access faster by aligning the types on their natural boundaries (equal to their size), the reason this works is the following:
*		- For 1 byte sized things, obviously they can be anywhere since the byte is the smallest unit. No matter where they are, you're going to have to fetch a whole word and cut out the
*			pieces that you don't want (because the CPU can only operate on words and not on single bytes (probably because efficiency, plus it saves on two/four wires to RAM)).
*		- For 2 byte sized things, you're going to have to fetch a word and cut out half, no matter where they are as well. You might think that this means the alignment on 2-sized things
*			doesn't matter, but it does, for this reason:
*				- If your variable is in the middle of a word (0XX0 <-- like that) and you stack another variable of same type next to it in an array, fetching both is going to require
*					2 word fetches instead of just one. That's why aligning 2-sized things on 2-byte boundaries (either XX00 or 00XX) is important, it prevents that.
*		- For all other sizes of things, it's very similar.
*
*	- Now, how is alignment handled inside structs and classes?
*		- Pretty much same thing, variables are aligned on their natural boundaries, but there is one more thing.
*		- At the end of the struct, padding is added so that the size of the struct is a multiple of the alignment of the variable with the biggest alignment inside of it.
*		- This insures that, if you put the struct in an array, all the alignment of every single variable never goes out of tune and everything stays super efficient.
*		- It's easy to see why this works.
* 
*	- IMPORTANT: This whole "alignment is equal to size of type" thing is only true for primitive types. Like I said, the alignment of a struct is equal to the alignment of the variable
*		inside of it with the biggest alignment.
*		--> Because of this, the alignment of a struct usually maxes out at 8 bytes, since there is almost no variable with an alignment bigger than that.
*			--> exceptions are: - if you use the alignof(x) specifier to put in custom alignment (or a variety of other (compiler specific) specifiers to mess with the alignment).
*								- If you use SSE instructions with 16 or 32 or something wide types. I think those often need to be aligned to boundaries of their size as well,
*									because they might be accessed with a "word" size that is bigger than the system word size.
* 
*/

struct Light {
	nmath::Vector3f position;
	alignas(16) nmath::Vector3f color;

	/*
	* 
	* The above usage of alignas is to make position and color take up the space of Vector4f's, so that the struct itself and packing it in arrays doesn't confuse the accel device code.
	* The great thing about this is that they are still accessible as Vector3f's on the host, so nothing has really changed in the rest of the code.
	* Following from the above notes on alignment, the alignment of this Light struct is 16.
	* 
	* NOTE: We shouldn't get any CPU efficiency decrease from this since aligning something on boundaries bigger than the optimal boundary isn't bad.
	* REASON: Every instance of an alignment on 16-byte boundary is also an instance of an alignment on 8-byte boundary.
	* So absolutely no problem here!
	* 
	*/
};
