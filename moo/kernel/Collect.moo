
class Collection(Object)
{
	method isEmpty
	{
		^self size <= 0
	}

	method notEmpty
	{
		^self size > 0
	}

	method size
	{
		(* Each subclass must override this method because
		 * it interates over the all elements for counting *)
		| count |
		count := 0.
		self do: [ :el | count := count + 1 ].
		^count
	}

	method add: object
	{
		self subclassResponsibility: #add:.
	}

	## ===================================================================
	## ENUMERATING
	## ===================================================================

	method do: block
	{
		^self subclassResponsibility: #do
	}

	method collect: block
	{
		| coll |
		##coll := self class new: self basicSize.
		coll := self class new.
		self do: [ :el | coll add: (block value: el) ].
		^coll
	}

	method detect: block
	{
		^self detect: block ifNone: [ ^self error: 'not found' ].
	}

	method detect: block ifNone: exception_block
	{
		## returns the first element for which the block evaluates to true.
		self do: [ :el | if (block value: el) { ^el } ].
		^exception_block value.
	}

	method select: condition_block
	{
		| coll |
		##coll := self class new: self basicSize.
		coll := self class new.
		self do: [ :el | if (condition_block value: el) { coll add: el } ].
		^coll
	}

	method reject: condition_block
	{
		| coll |
		##coll := self class new: self basicSize.
		coll := self class new.
		self do: [ :el | ifnot (condition_block value: el) { coll add: el } ].
		^coll
	}

	method emptyCheck
	{
		if (self size <= 0) { Exception signal: 'empty collection' }.
	}
}

## -------------------------------------------------------------------------------
class SequenceableCollection(Collection)
{
	method do: aBlock
	{
		0 to: (self size - 1) do: [:i | aBlock value: (self at: i)].
	}

	method reverseDo: aBlock
	{
		(self size - 1) to: 0 by: -1 do: [:i | aBlock value: (self at: i)].
	}
}

	## -------------------------------------------------------------------------------
class(#pointer) Array(SequenceableCollection)
{
	method size
	{
		^self basicSize
	}

	method at: anInteger
	{
		^self basicAt: anInteger.
	}

	method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}

	method first
	{
		^self at: 0.
	}

	method last
	{
		^self at: (self size - 1).
	}

	method copy: anArray
	{
		0 priorTo: (anArray size) do: [:i | self at: i put: (anArray at: i) ].
	}

	method copy: anArray from: start to: end
	{
		## copy elements from an array 'anArray' starting from 
		## the index 'start' to the index 'end'.

		| s i ss |

(*
		s := anArray size.

		if (start < 0) { start := 0 }
		elsif (start >= s) { start := s - 1 }.

		if (end < 0) { end := 0 }
		elsif (end >= s) { end := s - 1 }.
*)
		i := 0.
		ss := self size.
		while (start <= end)
		{
			if (i >= ss) { break }.
			self at: i put: (anArray at: start).
			i := i + 1.
			start := start + 1.
		}.
	}

	method copyFrom: start
	{
		| newsz |
		newsz := (self size) - start.
		^(self class new: newsz) copy: self from: start to: ((self size) - 1).
	}

	method copyFrom: start to: end
	{
		## returns a copy of the receiver starting from the element 
		## at index 'start' to the element at index 'end'.
		| newsz |
		newsz := end - start + 1.
		^(self class new: newsz) copy: self from: start to: end
	}

	method copyFrom: start count: count
	{
		^(self class new: count) copy: self from: start to: (start + count - 1)
	}

	method replaceFrom: start to: stop with: replacement
	{
		^self replaceFrom: start to: stop with: replacement startingAt: 0.
	}

	method replaceFrom: start to: stop with: replacement startingAt: rstart
	{
		| offset i |
		offset := rstart - start.
		i := start.
		while (i <= stop) 
		{ 
			self at: i put: (replacement at: i + offset).
			i := i + 1.
		}.
	}

	method replaceFrom: start count: count with: replacement
	{
		^self replaceFrom: start to: (start + count - 1) with: replacement startingAt: 0.
	}

	method replaceFrom: start count: count with: replacement startingAt: rstart
	{
		^self replaceFrom: start to: (start + count - 1) with: replacement startingAt: rstart
	}

	method = anArray
	{
		| size i |
		if (self class ~~ anArray class) { ^false }.

		size := self size.
		if (size ~~ anArray size) { ^false }.

		i := 0.
		while (i < size)
		{
			if ((self at: i) ~= (anArray at: i)) { ^false }.
			i := i + 1.
		}.

		^true.
	}

	method ~= anArray
	{
		^(self = anArray) not
	}
}

## -------------------------------------------------------------------------------

class(#character) String(Array)
{
	method & string
	{
		(* TOOD: make this a primitive for performance. *)
		
		(* concatenate two strings. *)
		| newsize newstr cursize appsize |

		cursize := self basicSize.
		appsize := string basicSize.
		newsize := cursize + appsize.
		(*newstr := self class basicNew: newsize.*)
		newstr := String basicNew: newsize.

		0 priorTo: cursize do: [:i | newstr at: i put: (self at: i) ].
		0 priorTo: appsize do: [:i | newstr at: (i + cursize) put: (string at: i) ].

		^newstr
	}

	method asString
	{
		^self
	}

	(* TODO: Symbol is a #final class. Symbol new is not allowed. To create a symbol programatically, you should
	 *       build a string and send asSymbol to the string............
	method asSymbol
	{
	}
	*)

	(* The strlen method returns the number of characters before a terminating null.
	 * if no terminating null character exists, it returns the same value as the size method *)
	method(#primitive,#lenient) _strlen.
	method(#primitive) strlen.
}

## -------------------------------------------------------------------------------

class(#character,#final,#limited,#immutable) Symbol(String)
{
	method asString
	{
		(* TODO: make this a primitive for performance *)

		(* convert a symbol to a string *)
		| size str i |
		size := self basicSize.
		str := String basicNew: size.

		##0 priorTo: size do: [:i | str at: i put: (self at: i) ].
		i := 0.
		while (i < size) 
		{ 
			str at: i put: (self at: i).
			i := i + 1
		}.
		^str.
	}

	method = anObject
	{
		(* for a symbol, equality check is the same as the identity check *)
		<primitive: #'Apex_=='>
		self primitiveFailed.
	}

	method ~= anObject
	{
		(* for a symbol, equality check is the same as the identity check *)
		<primitive: #'Apex_~~'>
		^(self == anObject) not.
	}
}

## -------------------------------------------------------------------------------

class(#byte) ByteArray(Array)
{
	## TODO: is it ok for ByteArray to inherit from Array?

##	method asByteString
##	{
##	}

	method decodeToCharacter
	{
		## TODO: support other encodings. it only supports utf8 as of now.
		<primitive: #_utf8_to_uc>
		self primitiveFailed(thisContext method).

		(*
		### TODO: implement this in moo also..
		| firstByte |
		firstByte := self at: 0.
		if ((firstByte bitAnd:2r10000000) == 0) { 1 }
		elsif (firstByte bitAnd:2r11000000) == 2r10000000) { 2 }
		elsif (firstByte bitAnd:2r11100000) == 2r11000000) { 3 }
		elsif (firstByte bitAnd:2r11110000) == 2r11100000) { 4 }
		elsif (firstByte bitAnd:2r11111000) == 2r11110000) { 5 }
		elsif (firstByte bitAnd:2r11111100) == 2r11111000) { 6 }.
		*)
	}
}

class(#byte) ByteString(Array)
{
}

## -------------------------------------------------------------------------------

class OrderedCollection(SequenceableCollection)
{
	var buffer.
	var firstIndex.
	var lastIndex. ## this is the last index plus 1.

	method(#class) new
	{
		^self new: 16.
	}

	method(#class) new: capacity
	{
		^super new __init_with_capacity: capacity.
	}

	method __init_with_capacity: capacity
	{
		self.buffer := Array new: capacity.
		self.firstIndex := capacity bitShift: -1.
		self.lastIndex := self.firstIndex.
	}

	method size
	{
		^self.lastIndex - self.firstIndex.
	}

	method capacity
	{
		^self.buffer size.
	}

	method at: index
	{
		| i |
		i := index + self.firstIndex.
		if ((i >= self.firstIndex) and (i < self.lastIndex)) { ^self.buffer at: index }.
		Exception signal: ('index ' & index asString & ' out of range').
	}

	method at: index put: obj
	{
		## replace an existing element. it doesn't grow the buffer.
		| i |
		i := index + self.firstIndex.
		if ((i >= self.firstIndex) and (i < self.lastIndex)) { ^self.buffer at: index put: obj }.
		Exception signal: ('index ' & index asString & ' out of range').
	}

	method first
	{
		self emptyCheck.
		^self.buffer at: self.firstIndex.
	}

	method last
	{
		self emptyCheck.
		^self.buffer at: self.lastIndex.
	}

	method addFirst: obj
	{
		if (self.firstIndex == 0) { self __grow_and_shift(8, 8) }.
		self.firstIndex := self.firstIndex - 1.
		self.buffer at: self.firstIndex put: obj.
		^obj.
	}

	method addLast: obj
	{
		if (self.lastIndex == self.buffer size) { self __grow_and_shift(8, 0) }.
		self.buffer at: self.lastIndex put: obj.
		self.lastIndex := self.lastIndex + 1.
		^obj.
	}

	method addAllFirst: coll
	{
		| coll_to_add |
		coll_to_add := if (self == coll) { coll shallowCopy } else { coll }.
		self __ensure_head_room(coll_to_add size).
		coll_to_add reverseDo: [:obj | 
			self.firstIndex := self.firstIndex - 1.
			self.buffer at: self.firstIndex put: obj.
		].
		^coll_to_add.
	}

	method addAllLast: coll
	{
		| coll_to_add |
		coll_to_add := if (self == coll) { coll shallowCopy } else { coll }.
		self __ensure_tail_room(coll_to_add size).
		coll_to_add do: [:obj |
			self.buffer at: self.lastIndex put: obj.
			self.lastIndex := self.lastIndex + 1.
		].
		^coll_to_add
	}

	method add: obj beforeIndex: index
	{
		self __insert_before_index(obj, index).
		^obj.
	}

	method add: obj afterIndex: index
	{
		self __insert_before_index(obj, index + 1).
		^obj.
	}

	method add: obj
	{
		^self addLast: obj.
	}

	method addAll: coll
	{
		coll do: [:el | self addLast: el ].
		^coll.
	}

	method removeFirst
	{
		| obj |
		self emptyCheck.
		obj := self.buffer at: self.firstIndex.
		self.buffer at: self.firstIndex put: nil.
		self.firstIndex := self.firstIndex + 1.
		^obj.
	}

	method removeLast
	{
		| obj li |
		self emptyCheck.
		li := self.lastIndex - 1.
		obj := self.buffer at: li.
		self.buffer at: li put: nil.
		self.lastIndex := li.
		^obj
	}

	method removeFirst: count
	{
		| r i |
		if ((count > self size) or (count < 0)) { Exception signal: 'removal count too big/small' }.
		r := Array new: count.
		i := 0.
		while (i < count)
		{
			r at: i put: (self.buffer at: self.firstIndex).
			self.buffer at: self.firstIndex put: nil.
			self.firstIndex := self.firstIndex + 1.
			i := i + 1.
		}.
		^r.
	}

	method removeLast: count
	{
		| r i li |
		if ((count > self size) or (count < 0)) { Exception signal: 'removal count too big/small' }.
		r := Array new: count.
		i := 0.
		while (i < count)
		{
			li := self.lastIndex - 1.
			r at: i put: (self.buffer at: li).
			self.buffer at: li put: nil.
			self.lastIndex := li.
			i := i + 1.
		}.
		^r
	}

	method removeAtIndex: index
	{
		## remove the element at the given position.
		| obj |
		obj := self at: index. ## the range is checed by the at: method.
		self __remove_index(index + self.firstIndex).
		^obj
	}

	method remove: obj ifAbsent: error_block
	{
		## remove the first element equivalent to the given object.
		## and returns the removed element.
		## if no matching element is found, it evaluates error_block
		## and return the evaluation result.
		| i cand |
		i := self.firstIndex.
		while (i < self.lastIndex)
		{
			cand := self.buffer at: i.
			if (obj = cand) { self __remove_index(i). ^cand }.
			i := i + 1.
		}.
		^error_block value.
	}

	## ------------------------------------------------
	## ENUMERATING
	## ------------------------------------------------
	method collect: aBlock
	{
		| coll |
		coll := self class new: self capacity.
		self do: [ :el | coll add: (aBlock value: el) ].
		^coll
	}

	method do: aBlock
	{
		##^self.firstIndex to: (self.lastIndex - 1) do: [:i | aBlock value: (self.buffer at: i)].

		| i |
		i := self.firstIndex.
		while (i < self.lastIndex)
		{
			aBlock value: (self.buffer at: i).
			i := i + 1.
		}.
	}

	method reverseDo: aBlock
	{
		| i |
		i := self.lastIndex.
		while (i > self.firstIndex)
		{
			i := i - 1.
			aBlock value: (self.buffer at: i).
		}.
	}

	method keysAndValuesDo: aBlock
	{
		| i |
		i := self.firstIndex.
		while (i < self.lastIndex)
		{
			aBlock value: (i - self.firstIndex) value: (self.buffer at: i).
			i := i + 1.
		}.
	}

	## ------------------------------------------------
	## PRIVATE METHODS
	## ------------------------------------------------
	method __grow_and_shift(grow_size, shift_count)
	{
		| newcon |
		newcon := (self.buffer class) new: (self.buffer size + grow_size).
		newcon replaceFrom: (self.firstIndex + shift_count) 
			to: (self.lastIndex - 1 + shift_count)
			with: self.buffer startingAt: (self.firstIndex).
		self.buffer := newcon.
		self.firstIndex := self.firstIndex + shift_count.
		self.lastIndex := self.lastIndex + shift_count.
	}

	method __ensure_head_room(size)
	{
		| grow_size |
		if (self.firstIndex < size)
		{
			grow_size := size - self.firstIndex + 8.
			self __grow_and_shift(grow_size, grow_size).
		}
	}

	method __ensure_tail_room(size)
	{
		| tmp |
		tmp := (self.buffer size) - self.lastIndex. ## remaining capacity
		if (tmp < size)
		{
			tmp := size - tmp + 8.  ## grow by this amount
			self __grow_and_shift(tmp, 0).
		}
	}

	method __insert_before_index(obj, index)
	{
		| i start pos cursize reqsize |

		if (index <= 0)
		{
			## treat a negative index as the indicator to 
			## grow space in the front side.
			reqsize := index * -1 + 1.
			if (reqsize >= self.firstIndex) { self __ensure_head_room(reqsize) }.
			self.firstIndex := self.firstIndex + index - 1.
			self.buffer at: self.firstIndex put: obj.
		}
		else
		{
			cursize := self size.
			if ((self.firstIndex > 0) and (index <= (cursize bitShift: -1)))
			{
				start := self.firstIndex - 1.
				self.buffer replaceFrom: start to: (start + index - 1) with: self.buffer startingAt: self.firstIndex.
				self.buffer at: (start + index)  put: obj.
				self.firstIndex := start.
			}
			else
			{
				reqsize := if (index > cursize) { index - cursize } else { 1 }.
				if (reqsize < 8) { reqsize := 8 }.
				self __grow_and_shift(reqsize + 8, 0).

				start := self.firstIndex + index.
				i := self.lastIndex.
				while (i > start)
				{
					self.buffer at: i put: (self.buffer at: i - 1).
					i := i - 1.
				}.
				pos := index + self.firstIndex.
				self.lastIndex := if (pos > self.lastIndex) { pos + 1 } else { self.lastIndex + 1 }.
				self.buffer at: pos put: obj.
			}.
		}.
	}

	method __remove_index(index)
	{
		## remove an element at the given index from the internal buffer's
		## angle. as it is for internal use only, no check is performed
		## about the range of index. the given index must be equal to or
		## grater than self.firstIndex and less than self.lastIndex.
		self.buffer
			replaceFrom: index
			to: self.lastIndex - 2
			with: self.buffer 
			startingAt: index + 1.
		self.lastIndex := self.lastIndex - 1.
		self.buffer at: self.lastIndex put: nil.
	}

	(*
	method findIndex: obj
	{
		| i |

		i := self.firstIndex.
		while (i < self.lastIndex)
		{
			if (obj = (self.buffer at: i)) { ^i }.
			i := i + 1.
		}.

		^Error.Code.ENOENT.
	} *)
}

## -------------------------------------------------------------------------------

class Set(Collection)
{
	var tally, bucket.

	method(#class) new
	{
		^self new: 16. ### TODO: default size.
	}

	method(#class) new: capacity
	{
		^super new __init_with_capacity: capacity.
	}

	method __init_with_capacity: capacity
	{
		if (capacity <= 0) { capacity := 2 }.
		self.tally := 0.
		self.bucket := Array new: capacity.
	}

	method isEmpty
	{
		^self.tally == 0
	}

	method size
	{
		^self.tally
	}

	method capacity
	{
		^self.bucket size.
	}

	method __make_expanded_bucket: bs
	{
		| newbuc newsz ass index i |

		(* expand the bucket *)
		newsz := bs + 16.  ## TODO: make this sizing operation configurable.
		newbuc := Array new: newsz.
		i := 0.
		while (i < bs)
		{
			ass := self.bucket at: i.
			if (ass notNil) 
			{
				index := (ass hash) rem: newsz.
				while ((newbuc at: index) notNil) { index := (index + 1) rem: newsz }.
				newbuc at: index put: ass
			}.
			i := i + 1.
		}.

		^newbuc.
	}

	method __find_index_for_add: anObject
	{
		| bs ass index |

		bs := self.bucket size.
		index := (anObject hash) rem: bs.

		while ((ass := self.bucket at: index) notNil)
		{
			if (anObject = ass) { ^index }.
			index := (index + 1) rem: bs.
		}.

		^index. ## the item at this index is nil.
	}

	method __find_index: anObject
	{
		| bs ass index |

		bs := self.bucket size.
		index := (anObject hash) rem: bs.

		while ((ass := self.bucket at: index) notNil)
		{
			if (anObject = ass) { ^index }.
			index := (index + 1) rem: bs.
		}.

		^Error.Code.ENOENT.
	}

	method __remove_at: index
	{
		| bs x y i v ass z |

		bs := self.bucket size.
		v := self.bucket at: index.

		x := index.
		y := index.
		i := 0.
		while (i < self.tally)
		{
			y := (y + 1) rem: bs.

			ass := self.bucket at: y.
			if (ass isNil) { (* done. the slot at the current index is nil *) break }.
			
			(* get the natural hash index *)
			z := (ass key hash) rem: bs.

			(* move an element if necessary *)
			if (((y > x) and ((z <= x) or (z > y))) or ((y < x) and ((z <= x) and (z > y))))
			{
				self.bucket at: x put: (self.bucket at: y).
				x := y.
			}.
			
			i := i + 1.
		}.
		
		self.bucket at: x put: nil.
		self.tally := self.tally - 1.
		
		(* return the affected association *)
		^v
	}

	method add: anObject
	{
		| index absent bs |
		
		## you cannot add nil to a set. however, the add: method doesn't
		## raise an exception for this. the includes: for nil returns
		## false naturally for the way it's implemented.
		if (anObject isNil) { ^anObject }.

		index := self __find_index_for_add: anObject.
		absent := (self.bucket at: index) isNil.
		self.bucket at: index put: anObject.

		if (absent) 
		{
			self.tally := self.tally + 1.
			bs := self.bucket size.
			if (self.tally > (bs * 3 div: 4)) { self.bucket := self __make_expanded_bucket: bs }.
		}.
		^anObject.
	}

	method remove: oldObject
	{
		| index |
		index := self __find_index: oldObject.
		if (index isError) { ^NotFoundException signal. }.
		^self __remove_at: index.
	}

	method remove: oldObject ifAbsent: anExceptionBlock
	{
		| index |
		index := self __find_index: oldObject.
		if (index isError) { ^anExceptionBlock value }.
		^self __remove_at: index.
	}

	method includes: anObject
	{
		^(self __find_index: anObject) notError.
	}

	method = aSet
	{
		ifnot (self class == aSet class) { ^false }.
		if (self == aSet){ ^true }.
		ifnot (self.tally = aSet size) { ^false }.
		self do: [ :el | ifnot (aSet includes: el) { ^false } ].
		^true
	}

	method do: aBlock
	{
		| bs i obj |
		bs := self.bucket size.
		i := 0.
		while (i < bs)
		{
			if ((obj := self.bucket at: i) notNil) { aBlock value: obj }.
			i := i + 1.
		}.
	}
	
	method collect: aBlock
	{
		## override the default implementation to avoid frequent growth
		## of the new collection being constructed. the block tends to 
		## include expression that will produce a unique value for each
		## element. so sizing the returning collection to the same size
		## as the receiver is likely to help. however, this assumption
		## isn't always true.

		| coll |
		coll := self class new: self capacity.
		self do: [ :el | coll add: (aBlock value: el) ].
		^coll
	}
}

class AssociativeCollection(Collection)
{
	var tally, bucket.

	method(#class) new
	{
		^self new: 16.
	}

	method(#class) new: capacity
	{
		^super new __init_with_capacity: capacity.
	}

	method __init_with_capacity: capacity
	{
		if (capacity <= 0) { capacity := 2 }.
		self.tally := 0.
		self.bucket := Array new: capacity.
	}

	method size
	{
		^self.tally
	}

	method capacity
	{
		^self.bucket size.
	}

	method __make_expanded_bucket: bs
	{
		| newbuc newsz ass index i |

		(* expand the bucket *)
		newsz := bs + 128. (* TODO: keep this growth policy in sync with VM(dic.c) *)
		newbuc := Array new: newsz.
		i := 0.
		while (i < bs)
		{
			ass := self.bucket at: i.
			if (ass notNil) 
			{
				index := (ass key hash) rem: newsz.
				while ((newbuc at: index) notNil) { index := (index + 1) rem: newsz }.
				newbuc at: index put: ass
			}.
			i := i + 1.
		}.

		^newbuc.
	}

	method __find: key or_upsert: upsert with: value
	{
		| hv ass bs index ntally |

		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key)
			{
				(* found *)
				if (upsert) { ass value: value }.
				^ass
			}.
			index := (index + 1) rem: bs.
		}.

		ifnot (upsert) { ^Error.Code.ENOENT }.

		ntally := self.tally + 1.
		if (ntally >= bs)
		{
			self.bucket := self __make_expanded_bucket: bs.
			bs := self.bucket size.
			index := hv rem: bs.
			while ((self.bucket at: index) notNil) { index := (index + 1) rem: bs }.
		}.

		ass := Association key: key value: value.
		self.tally := ntally.
		self.bucket at: index put: ass.

		^ass
	}

	method at: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		if (ass isError) { ^KeyNotFoundException signal: ('Unable to find ' & (key asString)) }.
		^ass value
	}

	method at: key ifAbsent: error_block
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		if (ass isError)  { ^error_block value }.
		^ass value
	}

	method associationAt: assoc
	{
		^self __find: (assoc key) or_upsert: false with: nil.
	}

	method associationAt: assoc ifAbsent: error_block
	{
		| ass |
		ass := self __find: (assoc key) or_upsert: false with: nil.
		if (ass isError) { ^error_block value }.
		^ass
	}

	method at: key put: value
	{
		## returns the affected/inserted association
		^self __find: key or_upsert: true with: value.
	}

	method includesKey: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		^ass notError
	}

	method includesAssociation: assoc
	{
		| ass |
		ass := self __find: (assoc key) or_upsert: false with: nil.
		^ass = assoc.
	}

	method includesKey: key value: value
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		^(ass key = key) and (ass value = value)
	}

	method __find_index: key
	{
		| bs ass index |

		bs := self.bucket size.
		index := (key hash) rem: bs.
		
		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key) { ^index }.
			index := (index + 1) rem: bs.
		}.

		^Error.Code.ENOENT.
	}

	method __remove_at: index
	{
		| bs x y i v ass z |

		bs := self.bucket size.
		v := self.bucket at: index.

		x := index.
		y := index.
		i := 0.
		while (i < self.tally)
		{
			y := (y + 1) rem: bs.

			ass := self.bucket at: y.
			if (ass isNil) { (* done. the slot at the current index is nil *) break }.
			
			(* get the natural hash index *)
			z := (ass key hash) rem: bs.

			(* move an element if necessary *)
			if (((y > x) and ((z <= x) or (z > y))) or ((y < x) and ((z <= x) and (z > y))))
			{
				self.bucket at: x put: (self.bucket at: y).
				x := y.
			}.
			
			i := i + 1.
		}.

		self.bucket at: x put: nil.
		self.tally := self.tally - 1.

		## return the affected association 
		^v
	}

	method removeKey: key
	{
		| index |
		index := self __find_index: key.
		if (index isError) { ^KeyNotFoundException signal: ('Unable to find ' & (key asString)). }.
		^self __remove_at: index.
	}

	method removeKey: key ifAbsent: error_block
	{
		| index |
		index := self __find_index: key.
		if (index isError) { ^error_block value }.
		^self __remove_at: index.
	}

	method removeAllKeys
	{
		(* remove all items from a dictionary *)
		| bs |
		bs := self.bucket size.
		0 priorTo: bs do: [:i | self.bucket at: i put: nil ].
		self.tally := 0
	}

(* TODO: ... keys is an array of keys.
	method removeAllKeys: keys
	{
		self notImplemented: #removeAllKeys:
	}
*)

	method remove: assoc
	{
		^self removeKey: (assoc key)
	}

	method remove: assoc ifAbsent: error_block
	{
		^self removeKey: (assoc key) ifAbsent: error_block
	}


	## ===================================================================
	## ENUMERATING
	## ===================================================================

	method collect: aBlock
	{
		| coll |
		coll := OrderedCollection new: self capacity.  
		self do: [ :el | coll add: (aBlock value: el) ].
		^coll
	}

	method select: aBlock
	{
		| coll |
		coll := self class new.
		## TODO: using at:put: here isn't really right. implement add: to be able to insert the assocication without
		##       creating another new association.
		##self associationsDo: [ :ass | if (aBlock value: ass value) { coll add: ass } ].
		self associationsDo: [ :ass | if (aBlock value: ass value) { coll at: (ass key) put: (ass value) } ].
		^coll
	}

	method do: aBlock
	{
		| bs i ass |
		bs := self.bucket size.
		i := 0.
		while (i < bs)
		{
			if ((ass := self.bucket at: i) notNil) { aBlock value: ass value }.
			i := i + 1.
		}.
	}

	method keysDo: aBlock
	{
		| bs i ass |
		bs := self.bucket size.
		i := 0.
		while (i < bs)
		{
			if ((ass := self.bucket at: i) notNil) { aBlock value: ass key }.
			i := i + 1.
		}.
	}

	method keysAndValuesDo: aBlock
	{
		| bs i ass |
		bs := self.bucket size.
		i := 0.
		while (i < bs)
		{
			if ((ass := self.bucket at: i) notNil)  { aBlock value: ass key value: ass value }.
			i := i + 1.
		}.
	}

	method associationsDo: aBlock
	{
		| bs i ass |
		bs := self.bucket size.
		i := 0.
		while (i < bs)
		{
			if ((ass := self.bucket at: i) notNil)  { aBlock value: ass }.
			i := i + 1.
		}.
	}
}

class SymbolTable(AssociativeCollection)
{
}

class Dictionary(AssociativeCollection)
{
	(* [NOTE] 
	 *  VM require Dictionary to implement new: and __put_assoc
	 *  for the dictionary expression notation - %{ }
	 *)
	
	## TODO: implement Dictionary as a Hashed List/Table or Red-Black Tree
	##       Do not inherit Set upon reimplementation
	##
	method(#class) new: capacity
	{
		^super new: (capacity + 10).
	}

	(* put_assoc: is called internally by VM to add an association
	 * to a dictionary with the dictionary/association expression notation
	 * like this:
	 *
	 *   %{ 1 -> 20, #moo -> 999 } 
	 *
	 * it must return self for the way VM works.
	 *)
	method __put_assoc: assoc
	{
		| hv ass bs index ntally key |

		key := assoc key.
		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		(* as long as 'assoc' supports the message 'key' and 'value'
		 * this dictionary should work. there is no explicit check 
		 * on this protocol of key and value. *)

		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key) 
			{
				(* found *)
				self.bucket at: index put: assoc.
				^self. ## it must return self for the instructions generated by the compiler.
			}.
			index := (index + 1) rem: bs.
		}.

		(* not found *)
		ntally := self.tally + 1.
		if (ntally >= bs) 
		{
			self.bucket := self __make_expanded_bucket: bs.
			bs := self.bucket size.
			index := hv rem: bs.
			while ((self.bucket at: index) notNil) { index := (index + 1) rem: bs }.
		}.

		self.tally := ntally.
		self.bucket at: index put: assoc.

		(* it must return self for the instructions generated by the compiler.
		 * otherwise, VM will break. *)
		^self.
	}
}

(* Namespace is marked with #limited. If a compiler is writeen in moo itself, it must 
 * call a primitive to instantiate a new namespace rather than sending the new message
 * to Namespace *)
class(#limited) Namespace(AssociativeCollection)
{
	var name, nsup.

	method name { ^self.name }
	## method name: name { self.name := name }

	(* nsup points to either the class associated with this namespace or directly
	 * the upper namespace placed above this namespace. when it points to a class,
	 * you should inspect the nsup field of the class to reach the actual upper
	 * namespace *)
	method nsup { ^self.nsup }
	## method nsup: nsup { self.nsup := nsup }

	method at: key
	{
		if (key class ~= Symbol) { InvalidArgumentException signal: 'key is not a symbol' }.
		^super at: key.
	}
	
	method at: key put: value
	{
		if (key class ~= Symbol) { InvalidArgumentException signal: 'key is not a symbol' }.
		^super at: key put: value
	}
}

class PoolDictionary(AssociativeCollection)
{
}

class MethodDictionary(AssociativeCollection)
{
}

extend Apex
{
	## -------------------------------------------------------
	## Association has been defined now. let's add association
	## creating methods
	## -------------------------------------------------------

	method(#class) -> object
	{
		^Association new key: self value: object
	}

	method -> object
	{
		^Association new key: self value: object
	}
}

## -------------------------------------------------------------------------------

class Link(Object)
{
	var prev, next, value.
	
	method(#class) new: value
	{
		| x | 
		x := self new.
		x value: value.
		^x
	}

	method prev { ^self.prev }
	method next { ^self.next }
	method value { ^self.value }
	method prev: link { self.prev := link }
	method next: link { self.next := link }
	method value: value { self.value := value }
}

class LinkedList(Collection)
{
	var first, last, tally.

	method initialize
	{
		self.tally := 0.
	}
	
	method size
	{
		^self.tally
	}

	method first
	{
		^self.first
	}

	method last
	{
		^self.last
	}

	method insertLink: link at: pos
	{
		if (pos isNil)
		{
			(* add link at the back *)
			if (self.tally == 0)
			{
				self.first := link.
				self.last := link.
				link prev: nil.
			}
			else
			{
				link prev: self.last.
				self.last next: link.
				self.last := link.
			}.
			link next: nil.
		}
		else
		{
			(* insert the link before pos *)
			link next: pos.
			link prev: pos prev.
			if (pos prev notNil) { pos prev next: link }
			else { self.first := link }.
			pos prev: link
		}.

		self.tally := self.tally + 1.
		^link
	}

	method insert: value at: pos
	{
		^self insertLink: (Link new: value) at: pos
	}
	
	method addFirst: value
	{
		^self insert: value at: self.first
	}
	
	method addLast: value
	{
		^self insert: value at: nil
	}

	method addFirstLink: link
	{
		^self insertLink: link at: self.first
	}
	
	method addLastLink: link
	{
		^self insertLink: link at: nil
	}
	
	method removeLink: link
	{
		if (link next notNil) { link next prev: link prev }
		else { self.last := link prev }.
		
		if (link prev notNil) { link prev next: link next }
		else { self.first := link next }.
		
		self.tally := self.tally - 1.
		^link.
	}

	method removeFirstLink
	{
		^self removeLink: self.first
	}

	method removeLastLink
	{
		^self removeLink: self.last
	}
	
	method do: block
	{
		| link next |
		link := self.first.
		while (link notNil)
		{
			next := link next.
			block value: link value.
			link := next.
		}
	}

	method doOverLink: block
	{
		| link next |
		link := self.first.
		while (link notNil)
		{
			next := link next.
			block value: link.
			link := next.
		}
	}

	method findEqualLink: value
	{
		self doOverLink: [:el | if (el value = value) { ^el }].
		^Error.Code.ENOENT
	}

	method findIdenticalLink: value
	{
		self doOverLink: [:el | if (el value == value) { ^el }].
		^Error.Code.ENOENT
	}
}
