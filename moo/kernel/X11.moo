#include 'Moo.moo'.

###var xxxx. ## global variables.

class X11(Object) from 'x11'
{
	(* The X11 class represents a X11 display *)

	var(#class) default_display.

	var cid.
	var windows. ## all windows registered

	var event_loop_sem, event_loop_proc.
	var ll_event_blocks.

	var expose_event.
	var key_event.
	var mouse_event.
	var mouse_wheel_event.

	method(#primitive) _connect.
	method(#primitive) _disconnect.
	method(#primitive) _get_fd.
	method(#primitive) _get_event.
	
	method cid { ^self.cid }
	method wid { ^nil }
	method display { ^self }
}

class X11.Exception(System.Exception)
{
}

class X11.Rectangle(Object)
{
	var x, y, width, height.

	method initialize
	{
		self.x := 0.
		self.y := 0.
		self.width := 0.
		self.height := 0.
	}
	
	method x { ^self.x }
	method y { ^self.y }
	method width { ^self.width }
	method height { ^self.height }

	method x: x { self.x := x }
	method y: y { self.y := y }
	method width: width { self.width := width }
	method height: height { self.height := height }

	method x: x y: y width: width height: height 
	{
		self.x := x.
		self.y := y.
		self.width := width.
		self.height := height.
	}
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
	var x, y, button.

	method x      { ^self.x }
	method y      { ^self.y }
	method button { ^self.button } ## X11.MouseButton

	method initialize
	{
		self.x := 0.
		self.y := 0.
		self.button := 0.
	}
	
	method x: x y: y button: button
	{
		self.x := x.
		self.y := y.
		self.button := button.
	}
}

class X11.MouseWheelEvent(X11.Event)
{
	var x, y, amount.
	
	method x { ^self.x }
	method y { ^self.y }
	method amount { ^self.amount }

	method initialize 
	{
		self.x := 0.
		self.y := 0.
		self.amount := 0.
	}
	
	method x: x y: y amount: amount
	{
		self.x := x.
		self.y := y.
		self.amount := amount.
	}
}

class X11.ExposeEvent(X11.Event)
{
	var x, y, width, height.

	method x { ^self.x }
	method y { ^self.y }
	method width { ^self.width }
	method height { ^self.height }

	method initialize
	{
		self.x := 0.
		self.y := 0.
		self.width := 0.
		self.height := 0.
	}

	method x: x y: y width: width height: height
	{
		self.x := x.
		self.y := y.
		self.width := width.
		self.height := height.
	}
}

## ---------------------------------------------------------------------------
## Graphics Context
## ---------------------------------------------------------------------------
pooldic X11.GCAttr
{
	(* see xcb/xproto.h *)
	GC_FOREGROUND := 4. 
	GC_BACKGROUND := 8.
	GC_LINE_WIDTH := 16.
	GC_LINE_STYLE := 32.
	GC_FONT       := 16384.
}

class X11.GC(Object) from 'x11.gc'
{
	var window, id.

	method(#primitive) _make (display, window).
	method(#primitive) _kill.
	method(#primitive) _drawLine(x1, y1, x2, y2).
	method(#primitive) _drawRect(x, y, width, height).
	method(#primitive) _fillRect(x, y, width, height).
	method(#primitive) _change(gcattr, value).

	method(#class) new: window
	{
		^(super new) __open_on: window
	}

	method __open_on: window
	{
		if ((id := self _make(window display cid, window wid)) isError) 
		{
			X11.Exception signal: 'cannot make a graphic context'
		}.

		self.window := window.
		self.id := id.
	}

	method close
	{
		if (self.id notNil)
		{
			self _kill.
			self.id := nil.
			self.window := nil.
		}
	}
	
	method foreground: value
	{
		^self _change (X11.GCAttr.GC_FOREGROUND, value)
	}

	method background: value
	{
		^self _change (X11.GCAttr.GC_BACKGROUND, value)
	}
}

## ---------------------------------------------------------------------------
## ---------------------------------------------------------------------------


class X11.Component(Object)
{
	var parent.

	method new
	{
		## you must call new: parent instead.
		self cannotInstantiate
	}

	method new: parent
	{
		## you must call new: parent instead.
		self cannotInstantiate
	}

	method parent
	{
		^self.parent
	}

	method parent: parent
	{
		self.parent := parent.
	}

	method display 
	{ 
		| p pp |
		p := self.
		while ((pp := p parent) notNil) { p := pp }.
		^p display.
	}

	method display: display
	{
		## do nothing
	}
}

class X11.Canvas(Component)
{
	method paint: gr
	{
		self subclassResponsibility: #paint
	}

	method update: gr
	{
		self subclassResponsibility: #paint
	}
}

class X11.WindowedComponent(Component) from 'x11.win'
{
	var(#class) geom.
	var wid, bounds.
	
	method(#primitive) _get_window_dwatom.
	method(#primitive) _get_window_id.
	method(#primitive) _make_window (display, x, y, width, height, parent).
	method(#primitive) _kill_window.
	method(#primitive) _get_window_geometry (geom).
	method(#primitive) _set_window_geometry (geom).
	
	method wid { ^self.wid }

	method initialize
	{
		self.bounds := X11.Rectangle new.
	}

	method(#class) new: parent
	{
		^(super new) __open_on_window: parent
	}

	method __open_on_window: parent
	{
		| id disp |

		disp := parent display.

		wid := self _make_window (disp cid, 5, 5, 300, 300, parent wid).
		if (wid isError) { X11.Exception signal: ('cannot make a window - ' & wid asString) }.
		
		self.wid := wid.
		self display: disp.

		if (disp ~= parent) 
		{
			self.parent := parent.
			parent addComponent: self.
		}.

		disp addWindow: self.

		self _get_window_geometry (self.bounds).
		self windowOpened.
	}

	method close
	{
		if (self.wid notNil)
		{
			self windowClosing.

			self display removeWindow: self.
			if (self.parent notNil) { self.parent removeComponent: self }.

			self _kill_window.
			self windowClosed. ## you can't make a primitive call any more

			self.wid := nil.
			self.parent := nil.
		}
	}

	method cachedBounds
	{
		^self.bounds
	}
	
	method bounds
	{
		self _get_window_geometry (self.bounds).
		^self.bounds
	}
	
	method bounds: rect
	{
		self _set_window_geometry (rect).
		self _get_window_geometry (self.bounds). ## To update bounds
	}

	method windowOpened
	{
	}
	method windowClosing
	{
	}
	method windowClosed
	{
	}
	method windowResized
	{
	}

	method expose: event
	{
	
	##	('EXPOSE  IN WINDOWS....' & (self.wid asString) & ' ' & (event x asString) & ' ' & (event y asString) & ' ' & (event width asString) & ' ' & (event height asString))  dump.
	}

	(* TODO: take the followings out to a seperate mouse listener??? *)
	method mouseEntered: event
	{
	}
	method mouseExited: event
	{
	}
	method mouseClicked: event
	{
	}
	method mousePressed: event
	{
		('MOUSE PRESSED' & (self.wid asString) & ' ' & (event x asString) & ' ' & (event y asString) & ' ' & (event button asString))   dump.
	}
	method mouseReleased: event
	{
		('MOUSE RELEASED' & (self.wid asString) & ' ' & (event x asString) & ' ' & (event y asString))  dump.
	}
	method mouseWheelMoved: event
	{
		('MOUSE WHEEL MOVED' & (self.wid asString) & ' ' & (event x asString) & ' ' & (event y asString) &   ' ' & (event amount asString))  dump.
	}
}

class X11.Container(WindowedComponent)
{
	var components.

	method initialize
	{
		super initialize.
		self.components := System.Dictionary new: 128.
	}

	method addComponent: component
	{
	## TODO: the key is component's wid. it's only supported for WindowedCompoennt.
	##       what is a better key to support Component that's not linked to any window?
		^self.components at: (component wid) put: component
	}

	method removeComponent: component
	{
		^self.components removeKey: (component wid)
	}
	
	method close
	{
		self.components do: [:c | c close ].
		^super close
	}
}

class X11.FrameWindow(Container)
{
	var display.

	method display { ^self.display }
	method display: display { self.display := display }
	
	method(#class) new
	{
		super new: X11 defaultDisplay. ## connect if X11 is not connected.
	}
	
	method(#class) new: display
	{
		^super new: display.
	}

	##method show: flag
	##{
	##}

	(*
	method windowActivated: event
	{
	}
	method windowDeactivated: event
	{
	}
	method windowIconified: event
	{
	}
	method windowDeiconified: event
	{
	}*)
}

class X11.Button(WindowedComponent)
{
}

extend X11
{
	method(#class) new
	{
		^(super new) __connect_to_display: nil.
	}

	method __connect_to_display: name
	{
	## TODO: make use of name to connect to a non-default display.
		| cid |
		cid := self _connect.
		if (cid isError) { X11.Exception signal: 'cannot connect to display' }.
		self.cid := cid.
	}

	method close
	{
		if (self.cid notNil)
		{
			self _disconnect.
			self.cid := nil.
		}
	}

	method(#class) defaultDisplay
	{
		| conn |
		if (self.default_display isNil)
		{
			conn := X11 new.
			##if (conn isError) { X11.Exception signal: 'cannot connect to default display' }.
			self.default_display := conn.
		}.
		
		^self.default_display
	}

	method(#class) closeDefaultDisplay
	{
		if (self.default_display notNil)
		{
			self.default_display close.
			self.default_display := nil.
		}
	}
	
	method(#class) enterEventLoop
	{
		^self defaultDisplay enterEventLoop
	}

	method initialize
	{
		super initialize.

		self.windows := System.Dictionary new: 100.
		
		self.expose_event := X11.ExposeEvent new.
		self.key_event := X11.KeyEvent new.
		self.mouse_event := X11.MouseEvent new.
		self.mouse_wheel_event := X11.MouseWheelEvent new.

		self.ll_event_blocks := System.Dictionary new.
		self.ll_event_blocks
			at: X11.LLEvent.KEY_PRESS         put: #__handle_key_event:;
			at: X11.LLEvent.KEY_RELEASE       put: #__handle_key_event:;
			at: X11.LLEvent.BUTTON_PRESS      put: #__handle_button_event:;
			at: X11.LLEvent.BUTTON_RELEASE    put: #__handle_button_event:;
			at: X11.LLEvent.MOTION_NOTIFY     put: #__handle_notify:;
			at: X11.LLEvent.ENTER_NOTIFY      put: #__handle_notify:;
			at: X11.LLEvent.LEAVE_NOTIFY      put: #__handle_notify:;
			at: X11.LLEvent.EXPOSE            put: #__handle_expose:;
			at: X11.LLEvent.DESTROY_NOTIFY    put: #__handle_destroy_notify:;
			at: X11.LLEvent.CONFIGURE_NOTIFY  put: #__handle_configure_notify:;
			at: X11.LLEvent.CLIENT_MESSAGE    put: #__handle_client_message:.
	}

	method connect
	{
		| cid |
		if (self.windows isNil)
		{
			if ((cid := self _connect) isError)  { ^cid }.
			self.cid := cid.
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
		^self.windows at: (window wid) put: window.
	}

	method removeWindow: window
	{
		^self.windows removeKey: (window wid)
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
							System logNl: ('Error while getting a event from display ' & self.cid asString).
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
			###self.even_loop_sem ...
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
		(*
			typedef struct xcb_expose_event_t {
				uint8_t      response_type;
				uint8_t      pad0;
				uint16_t     sequence;
				xcb_window_t window; // uint32_t
				uint16_t     x;
				uint16_t     y;
				uint16_t     width;
				uint16_t     height;
				uint16_t     count;
				uint8_t      pad1[2];
			} xcb_expose_event_t;
		*)

		| wid window |

		##wid := System _getUint32(event, 4). ## window
		wid := event getUint32(4).
		window := self.windows at: wid.

		if (window notError)
		{
			(*
			self.expose_event 
				x: System _getUint16(event, 8)        ## x
				y: System _getUint16(event, 10)       ## y
				width: System _getUint16(event, 12)   ## width
				height: System _getUint16(event, 14). ## height
			*)
			self.expose_event 
				x: event getUint16(8)        ## x
				y: event getUint16(10)       ## y
				width: event getUint16(12)   ## width
				height: event getUint16(14). ## height

			window expose: self.expose_event.
		}
		else
		{
			System logNl: ('Expose event on unknown window - ' & wid asString).
		}
	}

	method __handle_button_event: event
	{
		(*
			typedef uint8_t xcb_button_t;
			typedef struct xcb_button_press_event_t {
				uint8_t         response_type;
				xcb_button_t    detail;  // uint8_t
				uint16_t        sequence;
				xcb_timestamp_t time;    // uint32_t
				xcb_window_t    root;    // uint32_t
				xcb_window_t    event;
				xcb_window_t    child;
				int16_t         root_x;
				int16_t         root_y;
				int16_t         event_x;
				int16_t         event_y;
				uint16_t        state;
				uint8_t         same_screen;
				uint8_t         pad0;
			} xcb_button_press_event_t;
			typedef xcb_button_press_event_t xcb_button_release_event_t;
		*)

		| type wid window detail |
		type := System _getUint8(event, 0) bitAnd: 16r7F. ## lower 7 bits of response_type
		wid := System _getUint32(event, 12). ## event
		##type := event getUint8(0) bitAnd: 16r7F. ## lower 7 bits of response_type
		##wid := event getUint32(12). ## event
		
		window := self.windows at: wid.

		if (window notError)
		{
			##detail := System _getUint8(event, 1).
			detail := event getUint8(1).
			if (detail >= 1 and: [detail <= 3])
			{
				self.mouse_event 
					x: event getUint16(24)  ## event_x
					y: event getUint16(26)  ## event_y
					button: detail.
				
				if (type == X11.LLEvent.BUTTON_PRESS)
				{
					window mousePressed: self.mouse_event
				}
				else
				{
					window mouseReleased: self.mouse_event
				}
			}
			elsif (detail == 4 or: [detail == 5])
			{
				if (type == X11.LLEvent.BUTTON_RELEASE)
				{
					self.mouse_wheel_event
						x: event getUint16(24)   ## event_x
						y: event getUint16(26)   ## event_y
						amount: (if (detail == 4) { -1 } else { 1 }).
					window mouseWheelMoved: self.mouse_wheel_event
				}
			}
		}
		else
		{
			System logNl: ('Button event on unknown window - ' & wid asString).
		}
	}

	method __handle_destroy_notify: event
	{
		(*
		typedef struct xcb_destroy_notify_event_t {
			uint8_t      response_type;
			uint8_t      pad0;
			uint16_t     sequence;
			xcb_window_t event;
			xcb_window_t window;
		} xcb_destroy_notify_event_t;
		*)

		| wid window |

		wid := System _getUint32(event, 4). ## event
		window := self.windows at: wid.

		if (window notError)
		{
			'WINDOW DESTROYED....................' dump.
		}
		else
		{
			System logNl: ('Destroy notify event on unknown window - ' & wid asString).
		}
	}
	
	method __handle_configure_notify: event
	{
		(*
		typedef struct xcb_configure_notify_event_t {
			uint8_t      response_type;
			uint8_t      pad0;
			uint16_t     sequence;
			xcb_window_t event;
			xcb_window_t window;
			xcb_window_t above_sibling;
			int16_t      x;
			int16_t      y;
			uint16_t     width;
			uint16_t     height;
			uint16_t     border_width;
			uint8_t      override_redirect;
			uint8_t      pad1;
		} xcb_configure_notify_event_t;
		*)

		| wid window bounds width height |

		## type := System _getUint8(event, 0) bitAnd: 16r7F. ## lower 7 bits of response_type
		wid := System _getUint32(event, 4). ## event
		window := self.windows at: wid.

		if (window notError)
		{
			width := System _getUint16(event, 20).
			height := System _getUint16(event, 22).
			bounds := window cachedBounds. ## old bounds before resizing.
			if (bounds width ~= width or: [bounds height ~= height]) { window windowResized }.
		}
		else
		{
			System logNl: ('Configure notify event on unknown window - ' & wid asString).
		}
	}

	method __handle_client_message: event
	{
		(*
			typedef union xcb_client_message_data_t {
				uint8_t  data8[20];
				uint16_t data16[10];
				uint32_t data32[5];
			} xcb_client_message_data_t;

			typedef struct xcb_client_message_event_t {
				uint8_t                   response_type;
				uint8_t                   format;
				uint16_t                  sequence;
				xcb_window_t              window;    // uint32_t
				xcb_atom_t                type;      // uint32_t
				xcb_client_message_data_t data;
			} xcb_client_message_event_t;
		*)
		| type wid window dw |
		
		##wid := System _getUint32(event, 4). ## window
		wid := event getUint32(4). ## window
		window := self.windows at: wid.
		
		if (window notError)
		{
			##dw := System _getUint32(event, 12). ## data.data32[0]
			dw := event getUint32(12). ## data.data32[0]
			if (dw == window _get_window_dwatom)
			{
				window close.
			}
			
			## TODO: handle other client messages
		}
		else
		{
			System logNl: ('Client message on unknown window - ' & wid asString).
		}
	}

	method __handle_key_event: type
	{
		(*
			typedef struct xcb_key_press_event_t {
				uint8_t         response_type;
				xcb_keycode_t   detail;
				uint16_t        sequence;
				xcb_timestamp_t time;
				xcb_window_t    root;
				xcb_window_t    event;
				xcb_window_t    child;
				int16_t         root_x;
				int16_t         root_y;
				int16_t         event_x;
				int16_t         event_y;
				uint16_t        state;
				uint8_t         same_screen;
				uint8_t         pad0;
			} xcb_key_press_event_t;
			typedef xcb_key_press_event_t xcb_key_release_event_t;
		*)

		if (type = X11.LLEvent.KEY_PRESS)
		{
		}
		else
		{
		}
	}
}


class MyButton(X11.Button)
{
	method windowOpened
	{
		super windowOpened.
		self repaint.
	}

	method expose: event
	{
		super expose: event.
		self repaint.
	}

	method repaint
	{
		|gc|
		gc := X11.GC new: self.
		gc foreground: 16rFF8877.
		gc _fillRect(0, 0, 50, 50).
		gc close.
	}
}

class MyFrame(X11.FrameWindow)
{
	var gc.
	var b1.
	var b2.

	method windowOpened
	{
		super windowOpened.

		if (self.gc isNil)
		{
			self.gc := X11.GC new: self.
self.gc foreground: 10.
self.gc _drawLine(10, 20, 30, 40).
self.gc _drawRect(10, 20, 30, 40).
self.gc foreground: 20.
self.gc _drawRect(100, 100, 200, 200).
		}.
		
		self.b1 := MyButton new: self.

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
		| rect |

		super windowResized.

		rect := self bounds.
		rect x: 0; y: 0; height: ((rect height) quo: 2); width: ((rect width) - 2).
		self.b1 bounds: rect;
	}

	method expose: event
	{
		super expose: event.
		(*
		('EXPOSE....' & (self.id asString) & ' ' & (event x asString) & ' ' & (event y asString) & ' ' & (event width asString) & ' ' & (event height asString))  dump.
		
self.gc foreground: 2.
##self.gc drawLine: (10@20) to: (30@40).
self.gc _drawLine(10, 20, 300, 400).
self.gc _drawRect(10, 20, 30, 40).
self.gc foreground: 20.
self.gc _drawRect(100, 100, 200, 200).*)
	}
}




class MyObject(Object)
{	
	method(#class) abc
	{
	^9234554123489129038123908123908312908
	}
	
	method(#class) main 
	{
		| f q p disp1 disp2 disp3 |

		f := 20.
		(q:=:{ 10 -> 20, 20 -> 30, f + 40 -> self abc, (Association key: 10 value: 49), f -> f })dump.
		(q at: 60) dump.
		(q at: 10) dump.


		#( #a:y: #b #c) dump.
		f := LinkedList new.
		f addLast: 10.
		f addLast: 20.
		q := f addLast: 30.
		f addLast: 40.
		f removeLink: q.
		f addLastLink: q.
		(f findLink: 30) prev value dump.

		f do: [:v | v dump ].
		(f size asString & ' elements in list') dump.

		disp1 := X11 new.
		disp2 := X11 new.
		disp3 := X11 new.
		
		##X11 connect.
		f := MyFrame new: disp2.
		q := MyFrame new: disp1.
		p := MyFrame new: disp3.
		

	##	MyButton new: q.
	##	MyButton new: f.

(*
		f add: X11.Button new: 'click me'.
		f width: 200 height: 200.
		f show.
*)
		
		disp1 enterEventLoop. ## this is not a blocking call. it spawns another process.
		disp2 enterEventLoop.
		disp3 enterEventLoop.

		(*while (true)
		{
			'111' dump.
			##x signal_event_loop_semaphore.
			Processor sleepFor: 5.
		}.*)
		## [ while (true) { '111' dump. Processor sleepFor: 1. } ] fork.

		##X11 disconnect.
		
		##X11 closeDefaultDisplay.
	}
}
