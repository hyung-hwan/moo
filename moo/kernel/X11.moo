#include 'Moo.moo'.

class X11(Object) from 'x11'
{
	var display_base := nil.
	var shell_container := nil.

	var windows. ## all windows registered

	var event_loop_sem, event_loop_proc.
	var ll_event_blocks.

	var expose_event.
	var key_event.
	var mouse_event.
	var mouse_wheel_event.

	method(#class,#primitive,#liberal) _open_display(name).
	method(#class,#primitive) _close_display(display).
	method(#class,#primitive) _get_fd(display).
	method(#class,#primitive) _get_event(display, event).
	method(#class,#primitive) _create_window(display, window, x, y, width, height, fgcolor, bgcolor).

	method(#primitive) _get_evtbuf.
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
## X11 Widgets
## ---------------------------------------------------------------------------

class X11.Widget(Object)
{
	var(#get,#set)
		parent   := nil,
		x        := 0,
		y        := 0,
		width    := 0,
		height   := 0,
		fgcolor  := 0,
		bgcolor  := 0,
		realized := false.

	method displayOn: grctx
	{
	}

	method displayBase
	{
		if (self.parent isNil) { ^nil }.
		^self.parent displayBase.
	}

	method realize
	{
	}

	method dispose
	{
     }
}

class X11.Label(X11.Widget)
{
	var(#get) text := ''.

	method text: text
	{
		self.text := text.
		self displayOn: nil.
	}

	method displayOn: grctx
	{
		## grctx _fillRectangle ().
		## grctx _drawText (...).
		## TODO Draw Text...
	}

	method realize
	{
		## if i want to use a window to represent it, it must create window here.
		## otherwise, do other works in displayOn???
	}

	method dispose
	{
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
			selfns.Exception signal: 'Cannot add an already added widget'.
		}.

		self.children addLast: widget.
		widget parent: self.
	}

	method remove: widget
	{
		| link |
		if (widget parent =~ self)
		{
			selfns.Exception sinal: 'Cannot remove a foreign widget'
		}.

		## TODO: unmap and destroy...
		link := self.children findLink: widget.
		self.children removeLink: link.
		widget parent: nil.
	}

	method childrenCount
	{
		^self.children size
	}

	method dispose
	{
          self.children do: [:child | child dispose ]
	}
}

class X11.Shell(X11.Composite)
{
	var(#get) title.
	var(#get,#set) displayBase.

	method new: title
	{
		self.title := title.
	}

	method title: title
	{
		self.title := title.
		if (self.realized)
		{
			## set window title of this window.
		}
	}

	method realize
	{
		| wind |
		if (self.realized)  { ^self }.

		wind := X11 _create_window(self.displayBase, nil, self.x, self.y, self.width, self.height, self.fgcolor, self.bgcolor).
		if (wind isError)
		{
			self.Exception signal: ('Cannot create shell ' & self.title).
		}.

		self.children do: [:child | child realize ].
		self.realized := true.
	}
}

## ---------------------------------------------------------------------------
## X11 server
## ---------------------------------------------------------------------------
extend X11
{
	method(#class) new
	{
		^(super new) __connect_to: nil.
	}

	method __connect_to: name
	{
		| base |
		base := X11 _open_display(name).
		if (base isError) { self.Exception signal: 'cannot open display' }.
		self.display_base := base.
	}

	method dispose
	{
		if (self.display_event notNil)
		{
			X11 _free_event.
			self.display_event := nil.
		}.

		if (self.shell_container notNil)
		{
			self.shell_container dispose.
			self.shell_container := nil.
		}.

		if (self.display_base notNil)
		{

			X11 _close_display (self.display_base).
			self.display_base := nil.
		}l
	}

	method initialize
	{
		super initialize.

		self.shell_container := self.Composite new.
		self.display_event := self _alloc_event.

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

	method addShell: shell
	{
		if (shell displayBase isNil)
		{
			self.shell_container add: shell.
			shell displayBase: self.display_base.
		}
	}

	method removeShell: shell
	{
		if (shell displayBase notNil)
		{
			self.shell_container remove: shell.
			shell displayBase: nil.
		}
	}

	method registerWindow: window
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
			Processor signal: self.event_loop_sem onInput: (X11 _get_fd(self.display_base)).
			self.event_loop_proc := [
				| event ongoing |

				ongoing := true.
				while (self.shell_container childrenCount > 0)
				{
###'Waiting for X11 event...' dump.
					self.event_loop_sem wait.
					if (ongoing not) { break }.

					while (self _get_event(self.display_base, self.display_event))
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

				self dispose.
'CLOSING X11 EVENT LOOP' dump.
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




class MyObject(Object)
{
	var disp1, shell1, shell2.

	method main1
	{
		self.disp1 := X11 new.

		shell1 := (X11.Shell new title: 'Shell 1').
		shell2 := (X11.Shell new title: 'Shell 2').

		shell1 x: 10; y: 20; width: 100; height: 100.
		shell2 x: 200; y: 200; width: 200; height: 200.

		self.disp1 addShell: shell1.
		self.disp1 addShell: shell2.

		self.shell1 add: (X11.Label new text: 'xxxxxxxx').
		self.shell1 realize.
		self.shell2 realize.

		self.disp1 enterEventLoop. ## this is not a blocking call. it spawns another process.
	}

	method(#class) main
	{
		^self new main1
	}
}

