
#include 'Stix.st'.

#################################################################
## MAIN
#################################################################

## TODO: use #define to define a class or use #class to define a class.
##       use #extend to extend a class
##       using #class for both feels confusing.

#extend Apex
{

}

#extend SmallInteger
{
	#method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	#method inc
	{
		^self + 1.
	}
}

#class TestObject(Object)
{
	#dcl(#class) Q R.
	#dcl(#classinst) a1 a2.

	#method test999
	{
		^self.Q
	}
}

#class B.TestObject(Object)
{
	#dcl(#class) Q R.
	#dcl(#classinst) a1 a2.

	#method test000
	{
		^self.Q
	}
}

#pooldic ABC 
{
	#KKK := 20.
}


#class MyConsole(Console)
{
	#method box: origin corner: corner
	{
		| tmp |
		self setCursor: origin.
		self write: '+'.
		(corner x - origin x - 2) timesRepeat: [self write: '-'].
		self write: '+'.

		tmp := Point new.

		(origin y + 1) to: (corner y - 1) by: 1 do: [ :i |
			tmp x: origin x y: i.
			self setCursor: tmp.
			self write: '|'.

			tmp x: corner x.
			self setCursor: tmp.
			self write: '|'.
		].

		tmp x: origin x y: corner y.
		self setCursor: tmp.
		self write: '+'.
		(corner x - origin x - 2) timesRepeat: [self write: '-'].
		self write: '+'.
	}
}

#class MyObject(TestObject)
{
	#method(#class) main
	{
		| v1 v2 |
		System logNl: 'START OF MAIN'.

		v1 := MyConsole output.
		v1 clear.
		v1 box: 0@0 corner: 80@20.
		v1 write: S'hello, 월드 이거 좋지 않니\n'.
		v1 write: S'하하하하하하하하 좋아좋아 可愛くってしょうがない(^o^) ほのかちゃん、しおりちゃん元気そうだね！ 久しぶりに見た。しおりちゃんどうしたのかな？좋아 하라하하\n'.
		v1 close.

		System logNl: S'\0\0\0END OF MAIN\0AB\0\0\0C\0\0\0'.
	}

}

