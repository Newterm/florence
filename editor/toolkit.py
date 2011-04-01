#!/usr/bin/python
import cairo
import gtk
import copy

def abs(x):
	if x < 0:
		return -x
	else:
		return x

class object:
	def __init__(self, name, x, y, w, h):
		self.name = name
		self.x = x
		self.y = y
		self.w = w
		self.h = h
		# huddung vector
		self.dx = 0
		self.dy = 0
		# 0 = normal ; 1 = active ; 2 = selected
		self.status = 0
		self.moving = False
		self.offsetx = 0
		self.offsety = 0
		self.anchor = 0
		self.dirty = False

	def get_extents(self):
		return self.x, self.y, self.w, self.h

	def inbox(self, x, y, bx, by, bw=10, bh=10):
		x2 = bx + bw
		y2 = by + bh
		return ( x > bx ) and ( y > by ) and ( x < x2 ) and ( y < y2 )

	def hit(self, x, y):
		return self.inbox(x, y, self.x, self.y, self.w, self.h)

	def collide(self, ax, ay, ax2, ay2):
		if ax < ax2:
			x = ax
			x2 = ax2
		else:
			x = ax2
			x2 = ax
		if ay < ay2:
			y = ay
			y2 = ay2
		else:
			y = ay2
			y2 = ay
		ret = ( ( ( self.x <= x ) and ( (self.x+self.w) >= x ) ) or ( ( self.x >= x ) and ( self.x <= x2 ) ) )
		ret = ret and ( ( ( self.y <= y ) and ( (self.y+self.h) >= y ) ) or ( ( self.y >= y ) and ( self.y <= y2 ) ) )
		return ret

	def activate(self):
		if self.status < 1:
			self.dirty = True
			self.status = 1

	def deactivate(self):
		if self.status == 1:
			self.dirty = True
			self.status = 0

	def select(self):
		if self.status != 2:
			self.dirty = True
			self.status = 2

	def deselect(self):
		if self.status != 0:
			self.dirty = True
			self.status = 0

	def onpress(self, x, y):
		self.moving = True
		self.offsetx = x - self.x
		self.offsety = y - self.y
		# 173
		# 506
		# 284
		if ( self.offsetx <= 10 ) and ( self.offsety <= 10 ):
			self.anchor = 1
		elif ( self.offsetx <= 10 ) and ( self.offsety >= ( self.h - 10 ) ):
			self.anchor = 2
		elif ( self.offsety <= 10 ) and ( self.offsetx >= ( self.w - 10 ) ):
			self.anchor = 3
		elif ( self.offsetx >= ( self.w - 10 ) ) and ( self.offsety >= ( self.h - 10 ) ):
			self.anchor = 4
		elif self.inbox( self.offsetx, self.offsety, 0, (self.h/2)-5 ):
			self.anchor = 5
		elif self.inbox( self.offsetx, self.offsety, self.w-10, (self.h/2)-5 ):
			self.anchor = 6
		elif self.inbox( self.offsetx, self.offsety, (self.w/2)-5, 0 ):
			self.anchor = 7
		elif self.inbox( self.offsetx, self.offsety, (self.w/2)-5, self.h-10 ):
			self.anchor = 8
		else:
			self.anchor = 0

	def onrelease(self):
		self.moving = False
		if self.anchor == 1:
			self.x = self.x + self.dx
			self.y = self.y + self.dy
			self.w = self.w - self.dx
			self.h = self.h - self.dy
		elif self.anchor == 2:
			self.x = self.x + self.dx
			self.w = self.w - self.dx
			self.h = self.h + self.dy
		elif self.anchor == 3:
			self.y = self.y + self.dy
			self.w = self.w + self.dx
			self.h = self.h - self.dy
		elif self.anchor == 4:
			self.w = self.w + self.dx
			self.h = self.h + self.dy
		elif self.anchor == 5:
			self.x = self.x + self.dx
			self.w = self.w - self.dx
		elif self.anchor == 6:
			self.w = self.w + self.dx
		elif self.anchor == 7:
			self.y = self.y + self.dy
			self.h = self.h - self.dy
		elif self.anchor == 8:
			self.h = self.h + self.dy
		else:
			self.x = self.x + self.dx
			self.y = self.y + self.dy
		self.dx = 0
		self.dy = 0

	def onmotion(self, x, y):
		if self.moving:
			oldx = self.x
			oldy = self.y
			oldw = self.w
			oldh = self.h
			oldoffx = self.offsetx
			oldoffy = self.offsety
			# 173
			# 506
			# 284
			if self.anchor == 1:
				self.w = self.w + self.x
				self.x = x - self.offsetx
				self.w = self.w - self.x
				self.h = self.h + self.y
				self.y = y - self.offsety
				self.h = self.h - self.y
			elif self.anchor == 2:
				self.w = self.w + self.x
				self.x = x - self.offsetx
				self.w = self.w - self.x
				self.h = self.h - self.offsety
				self.offsety = y - self.y
				self.h = self.h + self.offsety
			elif self.anchor == 3:
				self.h = self.h + self.y
				self.y = y - self.offsety
				self.h = self.h - self.y
				self.w = self.w - self.offsetx
				self.offsetx = x - self.x
				self.w = self.w + self.offsetx
			elif self.anchor == 4:
				self.w = self.w - self.offsetx
				self.offsetx = x - self.x
				self.w = self.w + self.offsetx
				self.h = self.h - self.offsety
				self.offsety = y - self.y
				self.h = self.h + self.offsety
			elif self.anchor == 5:
				self.w = self.w + self.x
				self.x = x - self.offsetx
				self.w = self.w - self.x
			elif self.anchor == 6:
				self.w = self.w - self.offsetx
				self.offsetx = x - self.x
				self.w = self.w + self.offsetx
			elif self.anchor == 7:
				self.h = self.h + self.y
				self.y = y - self.offsety
				self.h = self.h - self.y
			elif self.anchor == 8:
				self.h = self.h - self.offsety
				self.offsety = y - self.y
				self.h = self.h + self.offsety
			else:
				self.x = x - self.offsetx
				self.y = y - self.offsety
			if self.w < 10:
				self.w = 10
				if self.x != oldx:
					self.x = oldx + oldw - 10
				if self.offsetx != oldoffx:
					self.offsetx = oldoffx - oldw + 10
			if self.h < 10:
				self.h = 10
				if self.y != oldy:
					self.y = oldy + oldh - 10
				if self.offsety != oldoffy:
					self.offsety = oldoffy - oldh + 10
			self.dirty = True
		elif self.hit(x, y):
			if self.status == 0:
				self.dirty = True
				self.status = 1
		else:
			if self.status == 1:
				self.dirty = True
				self.status = 0

	def onleave(self):
		if self.status == 1:
			self.dirty = True
			self.status = 0

	def draw(self, crctx):
		a = 1
		if self.moving:
			a = 0.7

		# hug
		if ( self.dx != 0 ) or ( self.dy != 0 ):
			tmp = cairo.ImageSurface(cairo.FORMAT_A8, 16, 16)
			cr2 = cairo.Context(tmp)
			cr2.set_source_rgba(0, 0, 0, 1)
			cr2.set_line_width(8)
			cr2.move_to(0, 0)
			cr2.line_to(16, 16)
			cr2.stroke()
			cr2.move_to(12, -4)
			cr2.line_to(20, 4)
			cr2.stroke()
			cr2.move_to(-4, 12)
			cr2.line_to(4, 20)
			cr2.stroke()
			pat = cairo.SurfacePattern(tmp)
			pat.set_extend(cairo.EXTEND_REPEAT)
			crctx.set_source(pat)
			# 173
			# 506
			# 284
			if self.anchor == 1:
				crctx.rectangle(self.x + self.dx + 2, self.y + self.dy + 2, self.w - 4 - self.dx, self.h - 4 - self.dy)
			elif self.anchor == 2:
				crctx.rectangle(self.x + self.dx + 2, self.y, self.w - 4 - self.dx, self.h - 4 + self.dy)
			elif self.anchor == 3:
				crctx.rectangle(self.x + 2, self.y + self.dy + 2, self.w - 4 + self.dx, self.h - 4 - self.dy)
			elif self.anchor == 4:
				crctx.rectangle(self.x + 2, self.y + 2, self.w - 4 + self.dx, self.h - 4 + self.dy)
			elif self.anchor == 5:
				crctx.rectangle(self.x + self.dx + 2, self.y + 2, self.w - 4 - self.dx, self.h - 4)
			elif self.anchor == 6:
				crctx.rectangle(self.x + 2, self.y + 2, self.w - 4 + self.dx, self.h - 4)
			elif self.anchor == 7:
				crctx.rectangle(self.x + 2, self.y + 2 + self.dy, self.w - 4, self.h - 4 - self.dy)
			elif self.anchor == 8:
				crctx.rectangle(self.x + 2, self.y + 2, self.w - 4, self.h - 4 + self.dy)
			else:
				crctx.rectangle(self.x + self.dx + 2, self.y + self.dy + 2, self.w - 4, self.h - 4)
			crctx.set_line_width(4)
			crctx.stroke()

		crctx.set_source_rgba(0.7, 0.7, 0.7, a)
		crctx.rectangle(self.x, self.y, self.w, self.h)
		crctx.fill()
		crctx.set_line_width(2)

		if self.status > 0:
			if self.status == 1:
				crctx.set_source_rgba(0.5, 0.5, 0.5, a)
			elif self.status == 2:
				crctx.set_source_rgba(0.1, 0, 0.5, a)
			crctx.rectangle(self.x, self.y, self.w, self.h)
			crctx.stroke()
			# corner anchors
			crctx.rectangle(self.x, self.y, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x+self.w-10, self.y, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x, self.y+self.h-10, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x+self.w-10, self.y+self.h-10, 10, 10)
			crctx.fill()
			#edge anchors
			crctx.rectangle(self.x, self.y+(self.h/2)-5, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x+self.w-10, self.y+(self.h/2)-5, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x+(self.w/2)-5, self.y, 10, 10)
			crctx.fill()
			crctx.rectangle(self.x+(self.w/2)-5, self.y+self.h-10, 10, 10)
			crctx.fill()
		else:
			crctx.set_source_rgba(0, 0, 0, 1)
			crctx.rectangle(self.x, self.y, self.w, self.h)
			crctx.stroke()
		xbearing, ybearing, width, height, xadvance, yadvance = crctx.text_extents ( self.name )
		crctx.move_to( self.x + ( self.w / 2 ) + 0.5 - xbearing - ( width / 2 ), self.y + (self.h / 2 ) + 0.5 - ybearing - ( height / 2 ) )
		crctx.set_source_rgba(0, 0, 0, 1)
		crctx.show_text(self.name)
		self.dirty = False

	def hug(self, dx, dy):
		self.dx = dx
		self.dy = dy

class selection:
	def __init__(self):
		self.objects = []
		self.active = None

	def get_extents(self):
		x = None
		y = None
		x2 = None
		y2 = None
		for sel in self.objects:
			if ( not x ) or ( x > sel.x ):
				x = sel.x	
			if ( not y ) or ( y > sel.y ):
				y = sel.y	
			if ( not x2 ) or ( x2 < ( sel.x + sel.w ) ):
				x2 = sel.x + sel.w
			if ( not y2 ) or ( y2 < ( sel.y + sel.h ) ):
				y2 = sel.y + sel.h
		return x, y, x2-x, y2-y

	def select(self, obj):
		try:
			self.objects.index(obj)
		except ValueError:
			self.clear()
			self.add(obj)

	def add(self, obj):
		self.objects.append(obj)
		obj.select()

	def remove(self, obj):
		try:
			self.objects.remove(obj)
			obj.deselect()
		except ValueError:
			pass

	def toggle(self, obj):
		try:
			self.objects.index(obj)
			self.remove(obj)
		except ValueError:
			self.add(obj)

	def motion(self, x, y):
		dirty = False
		for sel in self.objects:
			sel.onmotion(x, y)
			dirty = dirty or sel.dirty
		return dirty
	
	def press(self, obj, x, y):
		obj.onpress(x, y)
		for sel in self.objects:
			if sel != obj:
				sel.onpress(x, y)
				sel.anchor = obj.anchor

	def release(self):
		for sel in self.objects:
			sel.onrelease()

	def clear(self):
		mysel = copy.copy(self.objects)
		for sel in mysel:
			self.remove(sel)

	def hug(self, dx, dy):
		for sel in self.objects:
			sel.hug(dx, dy)

class scene:
	def totop(self, obj):
		self.top = obj
		self.objects.remove(obj)
		self.objects.append(obj)

	def setgrid(self, dx, dy):
		self.gridx = dx
		self.gridy = dy

	def leave(self, widget, event, data):
		for obj in self.objects:
			obj.onleave()
		widget.queue_draw()

	def motion(self, widget, event, data):
		dirty = False
		if self.top:
			dirty = self.sel.motion(event.x, event.y)
			# edge hugging
			self.sel.hug( 0, 0 )
			if event.state & gtk.gdk.MOD1_MASK:
				x, y, w, h = self.sel.get_extents()
				dx = 0
				dy = 0

				# align to grid
				if self.gridx != 0:
					if x % self.gridx <= 10:
						dx = -( x % self.gridx )
					if dx == 0 and -x % self.gridx <= 10:
						dx = ( -x ) % self.gridx
					if dx == 0 and ( ( x+w ) % self.gridx ) <= 10:
						dx = - ( ( x+w ) % self.gridx )
					if dx == 0 and ( ( -x-w ) % self.gridx ) <= 10:
						dx = ( ( -x-w ) % self.gridx )
				if self.gridy != 0:
					if y % self.gridy <= 10:
						dy = -( y % self.gridy )
					if dy == 0 and ( -y ) % self.gridy <= 10:
						dy = ( -y ) % self.gridy
					if dy == 0 and ( ( y+h ) % self.gridy ) <= 10:
						dy = - ( ( y+h ) % self.gridy )
					if dy == 0 and ( ( -y-h ) % self.gridy ) <= 10:
						dy = ( ( -y-h ) % self.gridy )
					
				for obj in self.objects.__reversed__():
					if obj.status == 2:
						continue
					x2, y2, w2, h2 = obj.get_extents()
					if ( dy == 0 ) and ( ( ( x2 <= x ) and ( (x2+w2) >= x ) ) or ( ( x2 >= x ) and ( (x+w) >= x2 ) ) ):
						if abs(y2+h2-y) <= 10:
							dy = y2+h2-y
						if (dy == 0) and (abs(y+h-y2) <= 10):
							dy = y2-y-h
					if ( dx == 0 ) and ( ( ( y2 <= y ) and ( (y2+h2) >= y ) ) or ( ( y2 >= y ) and ( (y+h) >= y2 ) ) ):
						if abs(x2+w2-x) <= 10:
							dx = x2+w2-x
						if (dx == 0) and (abs(x+w-x2) <= 10):
							dx = x2-x-w
					if ( dx != 0 ) and ( dy != 0 ):
						break
				if ( dx == 0 ) and ( x <= 10 ):
					dx = -x
				elif ( dx == 0 ) and ( ( x+w ) >= ( self.w - 10 ) ):
					dx = self.w - w - x
				if ( dy == 0 ) and ( y <= 10 ):
					dy = -y
				elif ( dy == 0 ) and ( ( y+h ) >= ( self.h - 10 ) ):
					dy = self.h - h - y
				self.sel.hug( dx, dy )
		else:
			if self.moving:
				self.xsel2 = event.x
				self.ysel2 = event.y
				dirty = True
				for obj in self.objects:
					if obj.collide(self.xsel, self.ysel, self.xsel2, self.ysel2):
						obj.activate()
					else:
						obj.deactivate()
			else:
				active = False
				for obj in self.objects.__reversed__():
					if active:
						obj.deactivate()
					else:
						obj.onmotion(event.x, event.y)
						active = obj.hit(event.x, event.y)
					dirty = dirty or obj.dirty
		if dirty:
			widget.queue_draw()

	def press(self, widget, event, data):
		hit = False
		for obj in self.objects.__reversed__():
			if obj.hit(event.x, event.y):
				hit = True
				if event.state & gtk.gdk.CONTROL_MASK:
					self.sel.toggle(obj)
				else:
					self.totop(obj)
					self.sel.select(obj)
				if obj.status == 2:
					self.sel.press(obj, event.x, event.y)
				break
		if not hit:
			if not (event.state & gtk.gdk.CONTROL_MASK):
				self.sel.clear()
			self.moving = True
			self.xsel = event.x
			self.ysel = event.y
			self.xsel2 = event.x
			self.ysel2 = event.y
		widget.queue_draw()

	def release(self, widget, event, data):
		if self.moving:
			for obj in self.objects:
				if obj.status == 1:
					self.sel.add(obj)
			self.moving = False
		else:
			self.sel.release()
		self.top = None
		widget.queue_draw()

	def expose(self, widget, event, data):
		crctx = widget.window.cairo_create()
		# draw grid
		crctx.set_source_rgba(0.7, 0.7, 0.7, 1)
		crctx.set_line_width(1)
		if self.gridx != 0:
			for x in range( 0, self.w, self.gridx ):
				crctx.move_to( x, 0 )
				crctx.line_to( x, self.h )
				crctx.stroke()
		if self.gridy != 0:
			for y in range( 0, self.h, self.gridy ):
				crctx.move_to( 0, y )
				crctx.line_to( self.w, y )
				crctx.stroke()
		for obj in self.objects:
			obj.draw(crctx)
		if self.moving:
			crctx.set_source_rgba(0.2, 0.2, 0.7, 0.1)
			crctx.rectangle(self.xsel, self.ysel, self.xsel2 - self.xsel, self.ysel2 - self.ysel)
			crctx.fill()
			crctx.set_source_rgba(0, 0, 0, 0.3)
			crctx.set_line_width(2)
			crctx.rectangle(self.xsel, self.ysel, self.xsel2 - self.xsel, self.ysel2 - self.ysel)
			crctx.stroke()

	def setSize(self, w, h):
		self.w = w
		self.h = h

	def __init__(self):
		self.reset()

	def connect(self, widget):
		self.handler1 = widget.connect("leave-notify-event", self.leave, self)
		self.handler2 = widget.connect("motion-notify-event", self.motion, self)
		self.handler3 = widget.connect("button-press-event", self.press, self)
		self.handler4 = widget.connect("button-release-event", self.release, self)
		self.handler5 = widget.connect("expose-event", self.expose, self)

	def disconnect(self, widget):
		widget.disconnect(self.handler1)
		widget.disconnect(self.handler2)
		widget.disconnect(self.handler3)
		widget.disconnect(self.handler4)
		widget.disconnect(self.handler5)

	def reset(self):
		self.objects = []
		self.sel = selection()
		self.top = None		
		self.moving = False
		self.xsel = 0
		self.ysel = 0
		self.xsel2 = 0
		self.ysel2 = 0
		self.gridx = 0
		self.gridy = 0

	def add(self, object):
		self.objects.append(object)

	def delete(self):
		for obj in self.sel.objects:
			self.objects.remove( obj );
		self.sel.clear()

