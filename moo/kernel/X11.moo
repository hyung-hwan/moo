#include "Moo.moo".

interface X11able
{
	method(#dual) abc.
	method(#dual,#liberal) def(x, y).
}
interface X11able2
{
	method(#dual) abc2.
	method(#dual) def.
}

interface X11able3
{
	method(#dual) class.
}

class QQQ(Object)
{
}

extend QQQ [X11able]
{

method(#dual) abc { ^nil }
method(#dual,#liberal) def(x, z) { ^nil }
}

class X11(Object) [X11able,selfns.X11able3] from "x11"
{
	// =====================================================================
	// this part of the class must match the internal
	// definition struct x11_t  defined in _x11.h
	// ---------------------------------------------------------------------
	var display_base := nil.
	// =====================================================================

	var shell_container := nil.
	var window_registry. // all windows registered

	var event_loop_sem, event_loop_proc.
	var llevent_blocks.
	var event_loop_exit_req := false.


method(#dual) abc { ^nil }
method(#dual,#liberal) def(x, z) { ^nil }
//#method(#dual) abc3 { ^nil }

	interface X11able3
	{
		method(#dual) abc55.
	}

	class Exception(System.Exception)
	{
	}

	class Point(Object)
	{
		var(#get,#set) x := 0, y := 0.
	}

	class Dimension(Object)
	{
		var(#get,#set) width := 0, height := 0.
	}

	class Rectangle(Object)
	{
		var(#get,#set)
			x      := 0,
			y      := 0,
			width  := 0,
			height := 0.
	}

	extend Point
	{
		method print
		{
			x dump.
			y dump.
		}
	}

/*
TODO: TODO: compiler enhancement
	class X11(Object)
	{
		class Rectangl(Object)
		{
		}
	}
	class XRect(X11.X11.Rectangl) -> X11 in X11.Rectangl is not the inner X11. as long as a period is found, the search begins at top.
	{
	}
	----> should i support soemthign like ::X11.Rectangle and X11.Rectangle? ::X11.Rectangle alwasy from the top???
	-----> or .X11.Rectangle -> to start search from the current name space???
*/

	method(#primitive,#liberal) _open_display(name).
	method(#primitive) _close_display.
	method(#primitive) _get_fd.
	method(#primitive) _get_llevent(llevent).

	method(#primitive) _create_window(parent_window_handle, x, y, width, height, fgcolor, bgcolor).
	method(#primitive) _destroy_window(window_handle).

	method(#primitive) _create_gc (window_handle).
	method(#primitive) _destroy_gc (gc). // note this one accepts a GC object.
	method(#primitive) _apply_gc (gc). // note this one accepts a GC object, not a GC handle.

	method(#primitive) _draw_rectangle(window_handle, gc_handle, x, y, width, height).
	method(#primitive) _fill_rectangle(window_handle, gc_handle, x, y, width, height).
	method(#primitive) _draw_string(gc, x, y, string).

	method __create_window(parent_window_handle, x, y, width, height, fgcolor, bgcolor, owner)
	{
		| w |
		w := self _create_window(parent_window_handle, x, y, width, height, fgcolor, bgcolor).
		if (w notError) { self.window_registry at: w put: owner }.
		^w
	}

	method __destroy_window(window_handle)
	{
		| w |
//#('DESTROY ' & window_handle asString) dump.
		w := self _destroy_window(window_handle).
		if (w notError) { self.window_registry removeKey: window_handle }
	}
}

// ---------------------------------------------------------------------------
// Event
// ---------------------------------------------------------------------------
pooldic X11.LLEventType
{
	KEY_PRESS         := 2.
	KEY_RELEASE       := 3.
	BUTTON_PRESS      := 4.
	BUTTON_RELEASE    := 5.
	MOTION_NOTIFY     := 6.
	ENTER_NOTIFY      := 7.
	LEAVE_NOTIFY      := 8.
	EXPOSE            := 12.
	VISIBILITY_NOTIFY := 15.
	DESTROY_NOTIFY    := 17.
	CONFIGURE_NOTIFY  := 22.
	CLIENT_MESSAGE    := 33.

	SHELL_CLOSE       := 65537.
}

class X11.LLEvent(Object)
{
	var(#get) type := 0, window := 0, x := 0, y := 0, width := 0, height := 0.
}

class X11.Event(Object)
{
}


class X11.KeyEvent(X11.Event)
{
}

pooldic X11.MouseButton
{
	LEFT := 1.
	MIDDLE := 2.
	RIGHT := 3.
}

class X11.MouseEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		button := 0. // X11.MouseButton
}

class X11.MouseWheelEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		amount := 0.
}

class X11.ExposeEvent(X11.Event)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		width  := 0,
		height := 0.
}



// ---------------------------------------------------------------------------
// X11 Context
// ---------------------------------------------------------------------------
pooldic X11.GCLineStyle
{
	SOLID       := 0.
	ON_OFF_DASH := 1.
	DOUBLE_DASH := 2.
}

pooldic X11.GCFillStyle
{
	SOLID           := 0.
	TILED           := 1.
	STIPPLED        := 2.
	OPAQUE_STIPPLED := 3.
}

class X11.GC(Object)
{
	// note these fields must match the x11_gc_t structure defined in _x11.h

	var(#get) widget := nil, gcHandle := nil.

	var(#get,#set)
		foreground := 0,
		background := 1,
		lineWidth  := 1,
		lineStyle  := X11.GCLineStyle.SOLID,
		fillStyle  := X11.GCFillStyle.SOLID,
		fontName   := nil.

	var fontPtr := nil.
	var fontSet := nil.

	method(#class) new
	{
		self messageProhibited: #new.
	}

	method(#class) new: widget
	{
		^(super new) __make_gc_on: widget
	}

	method __make_gc_on: widget
	{
		| gc |
widget displayServer dump.
widget windowHandle dump.
		gc := widget displayServer _create_gc (widget windowHandle).
		if (gc isError) { selfns.Exception signal: "Cannot create a graphics context" }.
		self.gcHandle := gc.
		self.widget := widget.
	}

	method drawRectangle(x, y, width, height)
	{
		^self.widget displayServer _draw_rectangle (self.widget windowHandle, self.gcHandle, x, y, width, height).
	}

	method fillRectangle(x, y, width, height)
	{
		^self.widget displayServer _fill_rectangle (self.widget windowHandle, self.gcHandle, x, y, width, height).
	}

	method drawString(x, y, string)
	{
		^self.widget displayServer _draw_string (self, x, y, string).
	}

	method dispose
	{
		if (self.gcHandle notNil)
		{
			self.widget displayServer _destroy_gc (self).
			self.gcHandle := nil
		}
	}

	method apply
	{
		if (self.gcHandle notNil)
		{
			if (self.widget displayServer _apply_gc (self) isError)
			{
				X11.Exception signal: "Cannot apply GC settings"
			}
		}
	}
}

// ---------------------------------------------------------------------------
// X11 Widgets
// ---------------------------------------------------------------------------

class X11.Widget(Object)
{
	var(#get) windowHandle := nil.

	var(#get,#set)
		parent   := nil,
		x        := 0,
		y        := 0,
		width    := 0,
		height   := 0,
		fgcolor  := 16r1188FF,
		bgcolor  := 0,
		realized := false.

	method displayServer
	{
		if (self.parent isNil) { ^nil }.
		^self.parent displayServer.
	}

	method parentWindowHandle
	{
		if (self.parent isNil) { ^nil }.
		^self.parent windowHandle.
	}

	method realize
	{
		| disp wind |

		if (self.windowHandle notNil)  { ^self }.

		disp := self displayServer.
		if (disp isNil)
		{
			X11.Exception signal: "Cannot realize a widget not added to a display server"
		}.

		wind := disp __create_window(self parentWindowHandle, self.x, self.y, self.width, self.height, self.fgcolor, self.bgcolor, self).
		if (wind isError)
		{
			self.Exception signal: "Cannot create widget window".
		}.

		self.windowHandle := wind.
	}


	method dispose
	{
		| disp |

'Widget dispose XXXXXXXXXXXXXX' dump.
		disp := self displayServer.
		if (disp notNil)
		{
			if (self.windowHandle notNil)
			{
				disp __destroy_window (self.windowHandle).
				self.windowHandle := nil.
			}.
		}
	}

	method onPaintEvent: paint_event
	{
	}

	method onMouseButtonEvent: event
	{
	}

	method onKeyEvent: event
	{
	}

	method onCloseEvent
	{
	}
}


class X11.Label(X11.Widget)
{
	var(#get) text := ''.

	method text: text
	{
		self.text := text.
		if (self windowHandle notNil) { self onPaintEvent: nil }
	}

	method realize
	{
		super realize.
	}

	method dispose
	{
'Label dispose XXXXXXXXXXXXXX' dump.
		super dispose.
	}

	method onPaintEvent: paint_event
	{
		| gc |

		gc := selfns.GC new: self.
		[
			gc foreground: self.bgcolor;
			   fontName: '-misc-fixed-medium-r-normal-ko-18-120-100-100-c-180-iso10646-1';
			   apply.
			gc fillRectangle (0, 0, self.width, self.height).

			gc foreground: self.fgcolor; apply.
			gc drawRectangle (0, 0, self.width - 1, self.height - 1).

			gc drawString(10, 10, self.text).
		] ensure: [ gc dispose ]
	}
}

class X11.Button(X11.Label)
{
	method onMouseButtonEvent: llevent
	{
		| type x |
		type := llevent type.
		if (type == X11.LLEventType.BUTTON_PRESS)
		{
			x := self.fgcolor.
			self.fgcolor := self.bgcolor.
			self.bgcolor := x.
			self onPaintEvent: llevent.
		}
		elif (type == X11.LLEventType.BUTTON_RELEASE)
		{
			x := self.fgcolor.
			self.fgcolor := self.bgcolor.
			self.bgcolor := x.
			self onPaintEvent: llevent.
		}
	}
}

class X11.Composite(X11.Widget)
{
	var children.

	method initialize
	{
		self.children := LinkedList new.
	}

	method add: widget
	{
		if (widget parent notNil)
		{
			selfns.Exception signal: "Cannot add an already added widget".
		}.

		self.children addLast: widget.
		widget parent: self.
	}

	method remove: widget
	{
		| link |
		if (widget parent ~~ self)
		{
			selfns.Exception signal: "Cannot remove an unknown widget"
		}.

		link := self.children findIdenticalLink: widget.
		self.children removeLink: link.
		widget parent: nil.
	}

	method childrenCount
	{
		^self.children size
	}

	method realize
	{
		super realize.
		[
			self.children do: [:child | child realize ].
		] on: System.Exception do: [:ex |
			self dispose.
			ex pass
		].
	}

	method dispose
	{
'Composite dispose XXXXXXXXXXXXXX' dump.
		self.children do: [:child |
			child dispose.
			self remove: child.
		].
		super dispose
	}

	method onPaintEvent: event
	{
		super onPaintEvent: event.
		self.children do: [:child |
			// TODO: adjust event relative to each child...
			child onPaintEvent: event.
		]
	}
}

class X11.Shell(X11.Composite)
{
	var(#get) title := ''.
	var(#get,#set) displayServer := nil.

	method new: title
	{
		self.title := title.
	}

//// TODO:
//// redefine x:, y:, width:, height: to return actual geometry values...
////

	method title: title
	{
		self.title := title.
		if (self.windowHandle notNil)
		{
			// set window title of this window.
		}
	}

	method realize
	{
'SHELL REALIZE XXXXXXXXXXX' dump.
		super realize.
	}

	method dispose
	{
'Shell dispose XXXXXXXXXXXXXX' dump.
		super dispose.
		self.displayServer removeShell: self.
	}

	method onCloseEvent
	{
'ON CLOSE EVENT .............' dump.
		self dispose.
	}
}

// ---------------------------------------------------------------------------
// X11 server
// ---------------------------------------------------------------------------
extend X11
{
	method(#class) new
	{
		^(super new) __connect_to: nil.
	}

	method __connect_to: name
	{
		if (self _open_display(name) isError)
		{
			self.Exception signal: 'cannot open display'
		}.
		self.display_base dump.
	}

	method isConnected
	{
		^self.display_base notNil
	}

	method dispose
	{
		if (self.shell_container notNil)
		{
			self.shell_container dispose.
			self.shell_container := nil.
		}.

		if (self.display_base notNil)
		{
			self _close_display.
			self.display_base := nil.
		}.
	}

	method initialize
	{
		super initialize.

		self.shell_container := self.Composite new.
		self.window_registry := System.Dictionary new: 100.

		/*

		self.llevent_blocks := System.Dictionary new.
		self.llevent_blocks
			at: self.LLEventType.KEY_PRESS         put: #__handle_key_event:on:;
			at: self.LLEventType.KEY_RELEASE       put: #__handle_key_event:on:;
			at: self.LLEventType.BUTTON_PRESS      put: #__handle_button_event:on:;
			at: self.LLEventType.BUTTON_RELEASE    put: #__handle_button_event:on:;
			at: self.LLEventType.MOTION_NOTIFY     put: #__handle_notify:on:;
			at: self.LLEventType.ENTER_NOTIFY      put: #__handle_notify:on:;
			at: self.LLEventType.LEAVE_NOTIFY      put: #__handle_notify:on:;
			at: self.LLEventType.EXPOSE            put: #__handle_expose:on:;
			at: self.LLEventType.DESTROY_NOTIFY    put: #__handle_destroy_notify:on:;
			at: self.LLEventType.CONFIGURE_NOTIFY  put: #__handle_configure_notify:on:;
			at: self.LLEventType.CLIENT_MESSAGE    put: #__handle_client_message:on:.
		*/
		self.llevent_blocks := ##{
			self.LLEventType.KEY_PRESS         -> #__handle_key_event:on:,
			self.LLEventType.KEY_RELEASE       -> #__handle_key_event:on:,
			self.LLEventType.BUTTON_PRESS      -> #__handle_button_event:on:,
			self.LLEventType.BUTTON_RELEASE    -> #__handle_button_event:on:,
			self.LLEventType.MOTION_NOTIFY     -> #__handle_notify:on:,
			self.LLEventType.ENTER_NOTIFY      -> #__handle_notify:on:,
			self.LLEventType.LEAVE_NOTIFY      -> #__handle_notify:on:,
			self.LLEventType.EXPOSE            -> #__handle_expose:on:,
			self.LLEventType.DESTROY_NOTIFY    -> #__handle_destroy_notify:on:,
			self.LLEventType.CONFIGURE_NOTIFY  -> #__handle_configure_notify:on:,
			self.LLEventType.CLIENT_MESSAGE    -> #__handle_client_message:on:,
			self.LLEventType.SHELL_CLOSE       -> #__handle_shell_close:on:
		}.
	}

	method addShell: shell
	{
		if (shell displayServer isNil)
		{
			self.shell_container add: shell.
			shell displayServer: self.
		}
	}

	method removeShell: shell
	{
		if (shell displayServer notNil)
		{
			self.shell_container remove: shell.
			shell displayServer: nil.
		}
	}

	method enterEventLoop
	{
		if (self.event_loop_sem isNil)
		{
			
			self.event_loop_sem := Semaphore new.
			self.event_loop_sem signalOnInput: (self _get_fd).
			self.event_loop_proc := [

				[
					| llevtbuf llevent ongoing |

					self.event_loop_exit_req := false.
					llevtbuf := X11.LLEvent new.
					ongoing := true.
					while (self.shell_container childrenCount > 0)
					{
'Waiting for X11 event...' dump.
						if  (self.event_loop_exit_req) { break }.
						self.event_loop_sem wait.
						if  (self.event_loop_exit_req) { break }.
						ifnot (ongoing) { break }.

						while ((llevent := self _get_llevent(llevtbuf)) notNil)
						{
							if (llevent isError)
							{
								//System logNl: ('Error while getting a event from server ' & self.cid asString).
								self.event_loop_exit_req := true.
								ongoing := false.
								break.
							}
							else
							{
								self __dispatch_llevent: llevent.
							}.
						}.
					}.
				] ensure: [

'CLOSING X11 EVENT LOOP' dump.

					//self.event_loop_sem signal. // in case the process is suspended in self.event_loop_sem wait.
					self.event_loop_sem unsignal.
			// TODO: LOOK HERE FOR RACE CONDITION with exitEventLoop.
					self.event_loop_sem := nil.
					self.event_loop_proc := nil.

					[ self dispose ] on: Exception do: [:ex | ("WARNING: dispose failure...." & ex messageText) dump ].
				]
			] fork.
		}
	}

	method exitEventLoop
	{
		if (self.event_loop_sem notNil)
		{
		// TODO: handle race-condition with the part maked 'LOOK HERE FOR RACE CONDITION'
			self.event_loop_proc terminate.
			self.event_loop_proc := nil.
			self.event_loop_sem := nil.
		}
	}

	method signal_event_loop_semaphore
	{
		self.event_loop_sem signal.
	}

	method requestToExit 
	{
		self.event_loop_exit_req := true.
		self.event_loop_sem signal.
	}

	method __dispatch_llevent: llevent
	{
		| widget mthname |

		widget := self.window_registry at: llevent window ifAbsent: [
			System logNl: 'Event on unknown widget - ' & (llevent window asString).
			^nil
		].

		mthname := self.llevent_blocks at: llevent type ifAbsent: [
			System logNl: 'Unknown event type ' & (llevent type asString).
			^nil
		].

		^self perform(mthname, llevent, widget).
	}

	method __handle_notify: llevent on: widget
	{
	}

	method __handle_expose: llevent on: widget
	{
		widget onPaintEvent: llevent
	}

	method __handle_button_event: llevent on: widget
	{
		widget onMouseButtonEvent: llevent
	}

	method __handle_destroy_notify: event on: widget
	{
	}

	method __handle_configure_notify: event on: widget
	{
		widget onPaintEvent: event.
	}

	method __handle_client_message: event on: widget
	{
		widget close: event.
	}

	method __handle_key_event: llevent on: widget
	{
		widget onKeyEvent: llevent
	}

	method __handle_shell_close: llevent on: widget
	{
		widget onCloseEvent.
	}
}


class Fx(Object)
{
	var(#class) X := 20.
	var x.

	method initialize
	{
		self.X := self.X + 1.
		self.x := self.X.
		self addToBeFinalized.
	}

	method finalize
	{
		System logNl: ('Greate... FX instance finalized' & self.x asString).
	}
}

class MyObject(Object)
{
	var disp1, disp2, shell1, shell2, shell3.
	var on_sig.

	method initialize
	{
		self.on_sig := [:sig | 
			self.disp1 requestToExit.
			self.disp2 requestToExit.
		].
	}

	method main1
	{
		| comp1 |

		self.disp1 := X11 new.
		self.disp2 := X11 new.

		shell1 := (X11.Shell new title: 'Shell 1').
		shell2 := (X11.Shell new title: 'Shell 2').

		shell3 := (X11.Shell new title: 'Shell 3').

		shell1 x: 10; y: 20; width: 500; height: 500.
		shell2 x: 200; y: 200; width: 200; height: 200.
		shell3 x: 500; y: 200; width: 200; height: 200.

		self.disp1 addShell: shell1.
		self.disp1 addShell: shell2.
		self.disp2 addShell: shell3.

		comp1 := X11.Composite new x: 10; y: 10; width: 400; height: 500.
		self.shell1 add: comp1.

		comp1 add: (X11.Label new text: '간다'; width: 100; height: 100).
		comp1 add: (X11.Label new text: 'crayon'; x: 90; y: 90; width: 100; height: 100).
	//	self.shell1 add: (X11.Label new text: 'xxxxxxxx'; width: 100; height: 100).
		self.shell1 add: (X11.Button new text: '크레용crayon'; x: 90; y: 90; width: 100; height: 100).

		self.shell2 add: (X11.Button new text: 'crayon'; x: 90; y: 90; width: 100; height: 100).
		self.shell3 add: (X11.Button new text: 'crayon'; x: 90; y: 90; width: 100; height: 100).
		self.shell1 realize.
		self.shell2 realize.
		self.shell3 realize.

		System installSignalHandler: self.on_sig.

		self.disp1 enterEventLoop. // this is not a blocking call. it spawns another process.
		self.disp2 enterEventLoop.

		comp1 := Fx new.
		Fx new.
		comp1 := Fx new.
		Fx new.

		while (self.disp1 isConnected or self.disp2 isConnected) { System sleepForSecs: 1 }.
	}

/*
	method exitEventLoops
	{
		if (self.disp2 notNil) { self.disp2 exitEventLoop }.
		if (self.disp1 notNil) { self.disp1 exitEventLoop }.
	}
*/

	method(#class) main
	{
		// this method returns immediately while having forked two processes with X11 event loops.
		^self new main1
	}
}

