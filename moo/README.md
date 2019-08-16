### Top-level elements
* #include
* #pragma
* class
* interface
* pooldic

### Comments

```
#! comment text
// comment text
/* comment text */
```

### Literal notations
* 200 decimal integer
* 2r1100 binary integer
* 16rFF00 hexadecimal integer
* 20p9999.99 fixed-point decimal where 20 is the number of digits after the point
* 999.9 fixed-point decimal

* $X character
* C'X' -> charcter??
* C"X" -> character??

* 'XXXX' string literal
* "XXXX" string litearl with escaping 

* B'XXXXXX' -> byte array literal
* B"XXXXX" -> byte array literal with escaping

* #XXXX symbol
* #'XXXX' quoted symbol
* #"XXXX" quoted symbol with escaping

* #\e123   Error literal
* #\pB8000000 SmallPointer(smptr) literal

* #() Array. Comma as element delimiter is optional
* #[] ByteArray. Comma as element delimiter is optional
* #{} Dictionary - not supported yet

The followings are not literals. The followings forms expressions.

* ##() Array expression. Comma required to delimit elements
* ##[] ByteArray expression. Comma required to delimit elements
* ##{} Dictionary expression. Comma required to delimit elements

The followings are not implemented. but can i do something like these?
* S#[] String literal with each character specified. S%{A B C '\n'}
* S#{} String with dynamic expression
* S#{ 65 asCharacter, 64 asCharacter }

### Class
```
class MyClass(Object)
{
	method show: this
	{
	}
}
```

### Flow Control
```
k := if (i < 20) { 30 } else { 40 }.

if (a < 10) { ... }
elif (a < 20) { ... }
else { ... }.

ifnot (i < 20) { 30 } else { 40 }. /* TODO: ifnot or nif? elifnot or elnif? */

if (a < 10) { .. } 
elifnot (a > 20) { ... }
else { ... }.
```

```
while (true)
{

}.

until (a > b)
{
}.

do
{
} while (a > b).

do
{
} until (a > b).

[a > b] whileTrue: [ ... ].
[a > b] whileFalse: [ ... ].
```

```
1 to: 20 do: [:count | ... ].
1 to: 10 by: 3 do: [:count | ... ].
30 timesRepeat: [ ... ].
```


### Exception handling
Exception signal.
Exception signal: "message".

[ ... ] on: Exception do: [:ex | ... ].

ex retry.
ex resume.
ex resume: value.
ex return: value.

### return from context(method/block)

explicit return operators
```
^ return_stacktop
^^ local_return
```

implicit return when the end of the context is reached
* a method context returns the receiver.
* a block context returns the last evaluated value. if nothing has been evaluated, nil is returned.

### goto

```
goto jump_label.

jump_label:
	a + b.
```

goto inside a block context.
```
[ 1 + 2. goto r. 3 + 4. r: 99 ]
```

goto must not cross the boundary of the block context.

```
this is invalid. cannot jump into a block from outside 
and vice versa.
 goto x.
 [ x:
   1 + 2 ].
```


### Type checking

Type checking not implemented yet.

```
class SampleClass(Object)
{
	method run => Integer
	{
	}

	method execute((Integer)a,(Integer)b) => Integer
	{
	}

	method handle: (Object)this with: (a Integer)care => Integer
	{
	}
}
```

TODO: How to specify return type of a block? or How to specify parameter type to a block?
      How to evaluate a block type-safely?

```
[ :(Integer)a :(Integer)b => Integer | 
	| (Integer)c (Integer)d }
	a + b 
] with: 20 with: 10

the method value is a variadic method. and it is a primitive.
[ :a :b | a + b ](1, 20) <---- syntax sugar for [:a :b | a + b] value(1, 20).

[:(Integer)a :(Integer)b => Integer | a + b ] value("kkkk", 20) 
 -> the first argument violates the expected type.
 -> argument types to a method is defined in the method signature.
 -> but the block argument types are specified in the block itself.
 -> so it doesn't seem natural to achieve type safety without treating 'value' specially.
 -> create a shorthand expression for 'value' [:(Integer)a :(Integer)b => (Integer) | a + b ](1, 2)
 -> ] followed by ( ===> shorthand expression for value.
 -> it looks more sensible to treat this () specially
 
what looks better as a shorthand expression for block value?
 [ :a :b | a + b ](10, 20)
 [ 10 + 20 ]() // no arguments
 [ :a :b | a + b ]->(10, 20). // -> cannot form a valid binary selector. must exclude it from binary selector patterns
 [ 10 + 20 ]->() // if no arugments are required.
 
 continuation?
 | a |
 a->()? block value?
 
 | (Block)a |
 a->()? block value?
 
```

### FFI 

```
	ffi := FFI new: 'libc.so.6'.
	[
		(ffi call: #printf signature: 's|sl>i' arguments: #("[%s ffi test %ld]\n" "sloppy" 12345678)) dump.
		(ffi call: #printf signature: 's>i' arguments: #("[%% ffi test %%]\n")) dump.
		(ffi call: #puts signature: 's>i' arguments: #("this is ffi test")) dump.
		(ffi call: #time signature: 'p>l' arguments: #(#\p0)) dump.
	]
	ensure: [
		ffi close.
	].
```




### Command line options

```
moo --log /dev/stderr ../kernel/test-001.moo
moo --log /dev/stderr,warn+ ../kernel/test-001.moo
```

