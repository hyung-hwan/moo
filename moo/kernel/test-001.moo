## TODO: use this file as a regress testing source.
##       rename it to test-001.moo and use it as a test case file.
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

	method(#class) main
	{
		| tc limit |
		
		tc := #{
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
			[MyObject.System.System.System KING == #KING]
		}.

		limit := tc size.
		MyObject.System.System a: XX.AAAA.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { ' PASS' } else { ' FAIL' }), S'\n').
		]
	}
}
