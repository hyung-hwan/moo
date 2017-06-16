#include 'Moo.moo'.

class X11(Object) from 'x11'
{
	var windows. ## all windows registered

	var event_loop_sem, event_loop_proc.
	var ll_event_blocks.

	var expose_event.
	var key_event.
	var mouse_event.
	var mouse_wheel_event.

	method(#primitive,#liberal) _connect(name).
	method(#primitive) _disconnect.
	method(#primitive) _get_base.
	method(#primitive) _get_fd.
	method(#primitive) _get_event.

	method server { ^self }
	method serverBase { ^self _get_base }
	method windowBase { ^nil }
}

class X11.Exception(System.Exception)
{
}

class X11.Point(Object)
{
	var(#get,#set) x := 0, y := 0.
}

class X11.Dimension(Object)
{
	var(#get,#set) width := 0, height := 0.
}

class X11.Rectangle(Object)
{
	var(#get,#set)
		x      := 0,
		y      := 0,
		width  := 0,
		height := 0.
}

## ---------------------------------------------------------------------------
## Event
## ---------------------------------------------------------------------------
pooldic X11.LLEvent
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
		button := 0. ## X11.MouseButton
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


## ---------------------------------------------------------------------------
## Window
## ---------------------------------------------------------------------------
class X11.Widget(Object)
{
	var(#get,#set) parent.

	method server
	{
		| p pp |
		p := self.
		while ((pp := p parent) notNil) { p := pp }.
		^p server.
	}

	method serverBase { ^self server serverBase }
	method windowBase { ^nil}


	##method displayOn: gc { }
}

class X11.Window(selfns.Widget) from 'x11.win'
{
	var bounds.

	method(#primitive) _make_window(server, x, y, width, height, parent).
	method(#primitive) _kill_window.
	method(#primitive) _get_base.
	method(#primitive) _get_bounds(rect).
	method(#primitive) _set_bounds(rect).

	method(#class) new: parent
	{
		^(super new) __open_on: parent.
	}

	method initialize
	{
		self.bounds := selfns.Rectangle new.
	}

	method __open_on: parent
	{
		| server winbase |

		server := parent server.
		winbase := self _make_window (server serverBase, 5, 5, 400, 400, parent windowBase).

		(*
		if (server _make_window (5, 5, 400, 400, parent, self) isError)
		{
			X11.Exception signal: 'cannot make a window'
		}.
		*)

		if (server ~= parent)
		{
			self.parent := parent.
			self.parent addWidget: self.
		}.

		server addWindow: self.

		self _get_bounds(self.bounds).
		self windowOpened.
	}

	method dispose
	{
		if (self windowBase notNil)
		{
			self windowClosing.

			self server removeWindow: self.
			if (self.parent notNil) { self.parent removeWidget: self }.

			self _dispose_window.
			self windowClosed. ## you can't make a primitive call any more

			##self.wid := nil.
			self.parent := nil.
		}
	}

	method windowBase { ^self _get_base }
}

class X11.FrameWindow(selfns.Window)
{
	method(#class) new: server
	{
		^super new: server.
	}
}

## ---------------------------------------------------------------------------
## X11 server
## ---------------------------------------------------------------------------
extend X11
{
	method(#class) new
	{
		^(super new) __connect_to_server: nil.
	}

	method __connect_to_server: name
	{
		| base |
		base := self _connect(name).
		if (base isError) { X11.Exception signal: 'cannot connect to server' }.
		##self.base := base.
	}

	method close
	{
		if (self _get_base notNil)
		{
			self _disconnect.
			##self.cid := nil.
		}
	}

	method initialize
	{
		super initialize.

		self.windows := System.Dictionary new: 100.

		self.expose_event := self.ExposeEvent new.
		self.key_event := self.KeyEvent new.
		self.mouse_event := self.MouseEvent new.
		self.mouse_wheel_event := self.MouseWheelEvent new.

		(*
		self.ll_event_blocks := System.Dictionary new.
		self.ll_event_blocks
			at: self.LLEvent.KEY_PRESS         put: #__handle_key_event:;
			at: self.LLEvent.KEY_RELEASE       put: #__handle_key_event:;
			at: self.LLEvent.BUTTON_PRESS      put: #__handle_button_event:;
			at: self.LLEvent.BUTTON_RELEASE    put: #__handle_button_event:;
			at: self.LLEvent.MOTION_NOTIFY     put: #__handle_notify:;
			at: self.LLEvent.ENTER_NOTIFY      put: #__handle_notify:;
			at: self.LLEvent.LEAVE_NOTIFY      put: #__handle_notify:;
			at: self.LLEvent.EXPOSE            put: #__handle_expose:;
			at: self.LLEvent.DESTROY_NOTIFY    put: #__handle_destroy_notify:;
			at: self.LLEvent.CONFIGURE_NOTIFY  put: #__handle_configure_notify:;
			at: self.LLEvent.CLIENT_MESSAGE    put: #__handle_client_message:.
		*)
		self.ll_event_blocks := %{
			self.LLEvent.KEY_PRESS         -> #__handle_key_event:,
			self.LLEvent.KEY_RELEASE       -> #__handle_key_event:,
			self.LLEvent.BUTTON_PRESS      -> #__handle_button_event:,
			self.LLEvent.BUTTON_RELEASE    -> #__handle_button_event:,
			self.LLEvent.MOTION_NOTIFY     -> #__handle_notify:,
			self.LLEvent.ENTER_NOTIFY      -> #__handle_notify:,
			self.LLEvent.LEAVE_NOTIFY      -> #__handle_notify:,
			self.LLEvent.EXPOSE            -> #__handle_expose:,
			self.LLEvent.DESTROY_NOTIFY    -> #__handle_destroy_notify:,
			self.LLEvent.CONFIGURE_NOTIFY  -> #__handle_configure_notify:,
			self.LLEvent.CLIENT_MESSAGE    -> #__handle_client_message:
		}.
	}

	method connect
	{
		| cid |
		if (self.windows isNil)
		{
			if ((cid := self _connect) isError)  { ^cid }.
			##self.cid := cid.
			self.windows := System.Dictionary new.
		}
	}

	method disconnect
	{
		if (self.windows notNil)
		{
			self.windows do: [ :frame |
				frame close.
			].
			self.windows := nil.
			self _disconnect.
		}
	}

	method addWindow: window
	{
		^self.windows at: (window windowBase) put: window.
	}

	method removeWindow: window
	{
		^self.windows removeKey: (window windowBase)
	}

	method enterEventLoop
	{
		if (self.event_loop_sem isNil)
		{
			self.event_loop_sem := Semaphore new.
			Processor signal: self.event_loop_sem onInput: (self _get_fd).
			self.event_loop_proc := [
				| event ongoing |

				ongoing := true.
				while (self.windows size > 0)
				{
###'Waiting for X11 event...' dump.
					self.event_loop_sem wait.
					if (ongoing not) { break }.

					while ((event := self _get_event) notNil)
					{
						if (event isError)
						{
							##System logNl: ('Error while getting a event from server ' & self.cid asString).
							ongoing := false.
							break.
						}
						else
						{
							self __dispatch_event: event.
						}.
					}.
				}.

				Processor unsignal: self.event_loop_sem.
				self.event_loop_sem := nil.

				self disconnect.
			] fork.
		}
	}

	method exitEventLoop
	{
		if (self.event_loop_sem notNil)
		{
			self.event_loop_proc terminate.
			self.event_loop_proc := nil.
			self.event_loop_sem := nil.
		}
	}

	method signal_event_loop_semaphore
	{
		self.event_loop_sem signal.
	}

	method __dispatch_event: event
	{
		| type mthname |

		##type := System _getUint8(event, 0).
		type := event getUint8(0).

		mthname := self.ll_event_blocks at: (type bitAnd: 16r7F).
		if (mthname isError)
		{
			(* unknown event. ignore it *)
('IGNORING UNKNOWN LL-EVENT TYPE ' & type asString) dump.
		}
		else
		{
			^self perform (mthname, event).
			##^self perform: mthname with: event.
		}
	}

	method __handle_notify: type
	{
		^9999999999
	}

	method __handle_expose: event
	{
	}

	method __handle_button_event: event
	{
	}

	method __handle_destroy_notify: event
	{
	}

	method __handle_configure_notify: event
	{
	}

	method __handle_client_message: event
	{
	}

	method __handle_key_event: type
	{
	}
}

class MyWidget(Window)
{
}

class MyFrame(X11.FrameWindow)
{
	var gc.


	method windowOpened
	{
		super windowOpened.

		(*
		if (self.gc isNil)
		{
			self.gc := X11.GC new: self.
self.gc foreground: 10.
self.gc _drawLine(10, 20, 30, 40).
self.gc _drawRect(10, 20, 30, 40).
self.gc foreground: 20.
self.gc _drawRect(100, 100, 200, 200).
		}.

		self.b1 := MyWidget new: self.*)

		self windowResized.
	}

	method windowClosing
	{
		super windowClosing.
		(*if (self.gc notNil)
		{
			self.gc close.
			self.gc := nil.
		}*)
	}

	method windowResized
	{
		(*
		| rect |

		super windowResized.

		rect := self bounds.
		rect x: 0; y: 0; height: ((rect height) quo: 2); width: ((rect width) - 2).
		self.b1 bounds: rect;*)
	}

	method expose: event
	{
		super expose: event.
	}
}

class MyObject(Object)
{

	method(#class) main
	{
		| disp1 disp2 disp3 f q p |

		disp1 := X11 new.
		disp2 := X11 new.
		disp3 := X11 new.

		f := MyFrame new: disp2.
		q := MyFrame new: disp1.
		p := MyFrame new: disp3.

		disp1 enterEventLoop. ## this is not a blocking call. it spawns another process.
		disp2 enterEventLoop.
		disp3 enterEventLoop.

		##disp1 := X11 new.
		##f := MyFrame new.
		##f add: Button new.
		##f add: Button new.
		##g := MyFrame new.
		##g add: (b := Button new).
		##disp1 add: f.
		##disp1 add: g.
		##disp1 enterEventLoop.
	}

}
