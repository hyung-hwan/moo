##
## TEST CASES for namespacing
##

#include 'Moo.moo'.


class MyObject(Object)
{
}

class MyObject.Donkey (MyObject)
{
	var(#class) Cat := MyObject, Party := 5555, X := 'test string'.
	
	method(#class) v { ^901982 }
}

pooldic MyObject.Code
{
	FAST := 20.
	FASTER := 40.
	FASTER2X := self.FASTER.
	FASTER3X := #(self.FAST self.FASTER self.FASTER2X #(MyObject.Donkey selfns.Donkey self.FAST FASTER) FASTER2X).
}

class MyObject.Donkey.Party (MyObject.Donkey)
{
}

class MyObject.Donkey.Party.Party (MyObject.Donkey)
{
}

extend MyObject.Donkey.Party
{
	method(#class) party1 { ^Party }
	method(#class) party2 { ^selfns.Party }
	method(#class) party3 { ^self.Party }
	method(#class) party4 { ^selfns.Party.Party }
	
	method(#class) cat1 { ^Cat }
	method(#class) code1 { ^MyObject.Code }
}

class MyObject.System.Donkey (MyObject)
{
	var t1, t2.
	method t1 { ^t1 }
	method(#class) v { ^89123 }
}

class MyObject.System.Horse (Donkey)
{
	method(#class) myns1 { ^selfns }
	method(#class) myns2 { [^selfns] value }
	
	method(#class) donkey1 { ^Donkey }
	method(#class) donkey2 { ^selfns.Donkey } ## should return same as MyObject.System.Donkey
	method(#class) donkey3 { ^MyObject.Donkey }
	
	method(#class) horse1 { ^Horse }
	method(#class) horse2 { ^selfns.Horse }
}

pooldic XX
{
	Horse := MyObject.System.Horse.
	Party := MyObject.Donkey.Party.
	AAAA := 'AAAAAAAAAAAAAAAAAAAAAAAAA'.
	FFFF := 'FFFFFFFFFFFFFFFFFFFFFFFFF'.
}

class MyObject.System.System (selfns.Donkey)
{
	var(#classinst,#get,#set) a, b, c.
	var(#get) ii := 0, jj := 0.
}
class MyObject.System.System.System (MyObject.System.System)
{
	var(#classinst,#get) d, dd, ddd.
	var(#class,#get) KING := #KING.
}

class MyObject.System.System.System.System (MyObject.System.System.System)
{
	var(#classinst,#get) e, f := XX.FFFF.
	var(#get) kk := #KK.
}

class MyObject.System.Stallion (selfns.Donkey)
{
	import System.XX.

	var(#set,#get) x := MyObject.Code.FASTER3X, rvd.
	var yyy.
	var(#set,#get) zebra, qatar := MyObject.Code.FASTER2X.
	

	method(#class) party { ^Party }
	method(#class) system1 { ^System } ## Single word. can be looked up in the current workspace.
	method(#class) system2 { ^System.System } ## Dotted. The search begins at the top.
}

extend MyObject
{
## TODO: support import in extend??

	method(#class) makeBlock(a,b)
	{
		^[:x | a * x + b]
	}

	method(#class) testMakeBlock
	{
		|a b |
		a := self makeBlock (12, 22).
		b := self makeBlock (99, 4).
                ^(a value: 5) * (b value: 6). ## (12 * 5 + 22) * (99 * 6 + 4) => 49036
	}

	method(#class) main
	{
		| tc limit |
		
		tc := %(
			## 0 - 4
			[MyObject.Donkey v == 901982],
			[selfns.MyObject.Donkey v == 901982],
			[MyObject.System.Donkey v == 89123],
			[MyObject.System.Horse v == 89123],
			[selfns.MyObject.System.Donkey v == 89123],
			
			## 5 - 9
			[MyObject.System.Horse donkey1 == MyObject.System.Donkey],
			[MyObject.System.Horse donkey2 == MyObject.System.Donkey],
			[MyObject.System.Horse donkey2 v == 89123],
			[MyObject.System.Horse donkey3 v == 901982],
			[MyObject.System.Horse horse1 == MyObject.System.Horse], 
			
			## 10 - 14
			[MyObject.System.Horse horse2 == MyObject.System.Horse],
			[MyObject.System.Horse superclass == MyObject.System.Donkey],
			[MyObject.System.Donkey superclass == MyObject],
			[selfns == System nsdic],
			[MyObject.System.Horse myns1 == MyObject.System],
			
			## 15 - 19
			[MyObject.System.Horse myns2 == MyObject.System],
			[MyObject.System == self.System],
			[MyObject.System.Donkey == self.System.Donkey],
			[MyObject.System.Stallion superclass == self.System.Donkey],
			[MyObject.Donkey.Party party1 == 5555],
			
			## 20 - 24
			[MyObject.Donkey.Party party2 == MyObject.Donkey.Party],
			[MyObject.Donkey.Party party3 == 5555],
			[MyObject.Donkey.Party party4 == MyObject.Donkey.Party.Party],
			[MyObject.Donkey.Party cat1 == MyObject],
			[MyObject.Code.FASTER2X == 40],
			
			## 25 - 29
			[MyObject.Code.FASTER2X == MyObject.Code.FASTER],
			[((MyObject.Code.FASTER3X at: 3) at: 1) == MyObject.Donkey],
			[MyObject.System.Stallion new x == MyObject.Code.FASTER3X],
			[MyObject.System.Stallion party == MyObject.Donkey.Party],
			[MyObject.System.Stallion system1 == MyObject.System.System],
			
			## 30 - 34
			[MyObject.System.Stallion system2 == System.System],
			[MyObject.System.System.System.System f == XX.FFFF],
			[MyObject.System.System a == XX.AAAA],
			[MyObject.System.System.System.System a == nil] ,
			[MyObject.System.System.System.System new kk == #KK],
			
			## 35 - 39
			[MyObject.System.System.System KING == #KING],
			[self testMakeBlock == 49036],
			[ ((-2305843009213693952 * -2305843009213693952 * 2305843009213693952 * 2305843009213693952 * 2305843009213693952) - 1 + 2) = 65185151242703554760590262029100101153646988597309960020356494379340201592426774597868716033 ],
			[ "%d" strfmt((-2305843009213693952 * -2305843009213693952 * 2305843009213693952 * 2305843009213693952 * 2305843009213693952) - 1 + 2) = '65185151242703554760590262029100101153646988597309960020356494379340201592426774597868716033' ],
			[ "%#b" strfmt((-2305843009213693952 * -2305843009213693952 * 2305843009213693952 * 2305843009213693952 * 2305843009213693952) - 1 + 2) = '2r100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001' ],

			## 40 - 44
			[ "%#x" strfmt((-2305843009213693952 * -2305843009213693952 * 2305843009213693952 * 2305843009213693952 * 2305843009213693952) - 1 + 2) = '16r20000000000000000000000000000000000000000000000000000000000000000000000000001' ],
			[ (7 div: -3) = -2 ],
			[ (7 rem: -3) = 1 ],
			[ (7 mdiv: -3) = -3 ],
			[ (7 mod: -3) = -2 ],

			## 45-49
			[ ([ (7 div: 0) = -2 ] on: Exception do: [:ex | ex messageText ]) = 'divide by zero' ],
			[ ([ (7 rem: 0) = -2 ] on: Exception do: [:ex | ex messageText ]) = 'divide by zero' ],
			[ ([ (7 mdiv: 0) = -2 ] on: Exception do: [:ex | ex messageText ]) = 'divide by zero' ],
			[ ([ (7 mod: 0) = -2 ] on: Exception do: [:ex | ex messageText ]) = 'divide by zero' ],
			[ (270000000000000000000000000000000000000000000000000000000000000000000 div:  50000000000000000000000000000000000000000000000000000000000000000000) = 5 ],

			## 50-54
			[ (270000000000000000000000000000000000000000000000000000000000000000000 rem:  50000000000000000000000000000000000000000000000000000000000000000000) = 20000000000000000000000000000000000000000000000000000000000000000000 ],
			[ (270000000000000000000000000000000000000000000000000000000000000000000 mdiv: 50000000000000000000000000000000000000000000000000000000000000000000) = 5],
			[ (270000000000000000000000000000000000000000000000000000000000000000000 mod:  50000000000000000000000000000000000000000000000000000000000000000000) = 20000000000000000000000000000000000000000000000000000000000000000000 ],
			[ (0 rem: -50000000000000000000000000000000000000000000000000000000000000000000) = 0 ],
			[ (0 rem: -50000000000000000000000000000000000000000000000000000000000000000000) = 0 ],

			## 55-59
			[ (0 rem: -50000000000000000000000000000000000000000000000000000000000000000000) = 0 ],
			[ (0 rem: -50000000000000000000000000000000000000000000000000000000000000000000) = 0 ],
			[ (16r2dd01fc06c265c8163ac729b49d890939826ce3dd div: 16r3b9aca00) = 4184734490355220618401938634485367707982 ],
			[ (16r2dd01fc06c265c8163ac729b49d890939826ce3dd rem: 16r3b9aca00) = 394876893 ],
			[ (16rFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF bitAnd: 16r1111111111111111111111111111111111111111) = 16r1111111111111111111111111111111111111111 ],

			## 60-64
			[(100213123891273912837891273189237 div: 1238971238971894573289472398477891263781263781263) = 0],
			[(100213123891273912837891273189237 rem: 1238971238971894573289472398477891263781263781263) = 100213123891273912837891273189237],
			[(-100213123891273912837891273189237 div: 1238971238971894573289472398477891263781263781263) = 0],
			[(-100213123891273912837891273189237 rem: 1238971238971894573289472398477891263781263781263) = -100213123891273912837891273189237],
			[(-100213123891273912837891273189237 mdiv: 1238971238971894573289472398477891263781263781263) = -1],

			## 65-69
			[(-100213123891273912837891273189237 mod: 1238971238971894573289472398477891263781263781263) = 1238971238971894473076348507203978425889990592026],
			[(-123897123897189421321312312321312312132 div: -123897123897189421321312312321312312132) = 1],
			[(-123897123897189421321312312321312312132 rem: -123897123897189421321312312321312312132) = 0],
			[(-123897123897189421321312312321312312132 mdiv: -123897123897189421321312312321312312132) = 1],
			[(-123897123897189421321312312321312312132 mod: -123897123897189421321312312321312312132) = 0],

			## 70-74
			[ (-0.1233 * 999999.123) = -123299.8918 ],
			[ (-0.1233 * 999999.123) asString =  '-123299.8918' ],
			[ (-0.1233 - -0.123) = -0.0003 ],
			[ (-0.1233 - -0.123) asString =  '-0.0003' ],
			[ (1.234 - 1.234) = 0 ], ## 0.000

			## 75-79
			[ (10.12 * 20.345) = 205.891 ],
			[ (10.12 mlt: 20.345) = 205.89 ],
			[ (-123897128378912738912738917.112323131233 div: 123.1) = -1006475453931053931053931.089458352000 ],
			[ (-1006475453931053931053931.089458352000 * 123.1) = -123897128378912738912738917.112323131200 ],
			[ 10 scale = 0 ],

			## 80-84
			[ 10.0 scale = 1 ],
			[ 10.00 scale = 2 ],
			[ (10 scale: 1) = 10.0 ],
			[ (10 scale: 1) scale = (10.1 scale)  ],
			[ (10 scale: 2) scale = (10.11 scale) ],

			## 85-89
			[ ((-10.19 scale: 3) scale) = (10.199 scale) ],
			[ ((-10.19 scale: 0) scale) = (10 scale) ],
			[ (-9p10 scale) = (-10.000000000 scale) ],
			[ (-9p10.123 scale) = (-10.123000000 scale) ],
			[ (+3p100.1 + 16rffff + +5p1.22 + -5p1.223) = 65635.09700 ],

			## =========================
			[
				| b |
				b := [:n | (n > 0) ifTrue: [ n * (b value: n - 1)] ifFalse: [1]].
				(b value: 3) == 6
			],
			[
				| b |
				b := [:n | (n > 0) ifTrue: [ (b value: n - 1) * n] ifFalse: [1]].
				(b value: 3) == 6
			]
		).

		limit := tc size.
		MyObject.System.System a: XX.AAAA.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		].
	}
}
