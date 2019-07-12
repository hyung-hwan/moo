### Top-level elements
* #include
* #pragma
* class
* interface
* pooldic

### Comments

~~~
#! comment text
// comment text
/* comment text */
~~~

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

The following are not literals. The followings forms expressions.

* ##() Array expression. Comma required to delimit elements
* ##[] ByteArray expression. Comma required to delimit elements
* ##{} Dictionary expression. Comma required to delimit elements

* S%[] String literal with each character specified 
** S%{A B C '\n'}

* S%{} String with dynamic expression
* S%{ 65 asCharacter, 64 asCharacter }


### Class

~~~
class MyClass(Object)
{
	method show: this
	{
	}
}
~~~

### Flow Control
~~~
k := if (i < 20) { 30 } else { 40 }.
~~~

~~~
while (true)
{

}.
~~~

~~~
1 to: 20 do: [:count | ... ].
~~~


### Exception handling
Exception signal.
Exception signal: "message".

[ ... ] on: Exception do: [:ex | ... ].

ex retry.
ex resume.
ex resume: value.
ex return: value.
