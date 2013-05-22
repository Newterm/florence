var offset = -1;
$.fn.insertAtCaret = function(myValue) {
	return this.each(function() {
		var me = this;
		if (document.selection) { // IE
			me.focus();
			sel = document.selection.createRange();
			sel.text = myValue;
			me.focus();
		} else if (me.selectionStart || me.selectionStart == '0') { // Real browsers
			var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
			me.value = me.value.substring(0, startPos) + myValue + me.value.substring(endPos, me.value.length);
			me.focus();
			me.selectionStart = startPos + myValue.length;
			me.selectionEnd = startPos + myValue.length;
			me.scrollTop = scrollTop;
		} else {
			me.value += myValue;
			me.focus();
		}
		offset = -1;
	});
};

$.fn.left = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		me.focus();
		me.selectionStart = startPos - 1;
		me.selectionEnd = startPos - 1;
		me.scrollTop = scrollTop;
		offset = -1;
	});
};

$.fn.right = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		me.focus();
		me.selectionStart = endPos + 1;
		me.selectionEnd = endPos + 1;
		me.scrollTop = scrollTop;
		offset = -1;
	});
};

$.fn.up = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		me.focus();
		var beginStr = me.value.substring(0, startPos);
		var index = beginStr.lastIndexOf("\n");
		if (index > -1) {
			if (offset == -1 || me.value.substring(startPos,startPos+1) != '\n') {
				offset = startPos - index;
			}
			beginStr = me.value.substring(0, index);
			var index2 = beginStr.lastIndexOf("\n");
			if ( (index2 + offset) < index ) me.selectionStart = index2 + offset;
			else me.selectionStart = index;
			me.selectionEnd = me.selectionStart;
			me.scrollTop = scrollTop;
		}
	});
};

$.fn.down = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		me.focus();
		var beginStr = me.value.substring(0, endPos);
		var index = beginStr.lastIndexOf("\n");
		if (offset == -1 || me.value.substring(endPos,endPos+1) != '\n') {
			offset = endPos + 1;
			if (index > -1) {
				offset = endPos - index;
			}
		}
		var endStr = me.value.substring(endPos);
		var index = endStr.search("\n");
		if (index > -1) {
			index2 = endStr.substring(index + 1, index + offset).search("\n");
			if (index2 > -1) {
				me.selectionStart = endPos + index + index2 + 1;
			} else me.selectionStart = endPos + index + offset;
			me.selectionEnd = me.selectionStart;
			me.scrollTop = scrollTop;
		}
	});
};

$.fn.backSpace = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		if (startPos == endPos) {
			me.value = me.value.substring(0, startPos-1) + me.value.substring(endPos, me.value.length);
			me.focus();
			me.selectionStart = startPos - 1;
			me.selectionEnd = startPos - 1;
		} else {
			me.value = me.value.substring(0, startPos) + me.value.substring(endPos, me.value.length);
			me.focus();
			me.selectionStart = startPos;
			me.selectionEnd = startPos;
		}
		me.scrollTop = scrollTop;
		offset = -1;
	});
};

$.fn.tab = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		if (startPos == endPos) {
			$(me).insertAtCaret("\t");
		} else {
			var m = me.value.substring(startPos, endPos).match(/\n/g); 
			var count = 1
			if (m !== null) count += m.length; 
			var beginStr = me.value.substring(0, startPos);
			if (me.value[startPos] == '\n') {
				count -= 1;
			} else if (beginStr.search("\n") > - 1) {
				var index = beginStr.lastIndexOf("\n");
				beginStr = me.value.substring(0, index+1) + "\t" +  me.value.substring(index+1, startPos);
			} else {
				beginStr = "\t" + me.value.substring(0, startPos);
			}
			me.value = beginStr +
			      me.value.substring(startPos, endPos).replace(/\n/g, "\n\t") +
			      me.value.substring(endPos, me.value.length);
			me.focus();
			if (me.value[startPos] != '\t') startPos += 1;
			me.selectionStart = startPos;
			me.selectionEnd = endPos+count;
			me.scrollTop = scrollTop;
		}
		offset = -1;
	});
};

$.fn.leftTab = function() {
	return this.each(function() {
		var me = this;
		var startPos = me.selectionStart, endPos = me.selectionEnd, scrollTop = me.scrollTop;
		if (startPos == endPos) {
			var beginStr = me.value.substring(0, startPos);
			var index = -1;
			if (beginStr.search("\n") > - 1) {
				index = beginStr.lastIndexOf("\n\t");
				if (index > -1) index += 1;
			} else {
				index = (beginStr.substring(0,1) == '\t'?0:-1)
			}
			if (index > -1) {
				beginStr = me.value.substring(0, index) + me.value.substring(index+1, startPos);
				startPos -= 1;
			}
			me.value = beginStr + me.value.substring(endPos, me.value.length);
			me.focus();
			me.selectionStart = startPos;
			me.selectionEnd = startPos;
		} else {
			var m = me.value.substring(startPos, endPos).match(/\n\t/g);
			var count = 0
			if (m !== null) count += m.length; 
			var beginStr = me.value.substring(0, startPos);
			var offset = -1;
			if (beginStr.search("\n") > -1) {
				var index = beginStr.lastIndexOf("\n");
				if (((index+1) == startPos) && (me.value[startPos] == '\t')) {
					startPos += 1;
					offset += 1;
				} else if (me.value[index+1] == '\t') {
					beginStr = me.value.substring(0, index+1) + me.value.substring(index+2, startPos);
					offset += 1;
				}
			} else {
				if (beginStr[0] == '\t') {
					beginStr = me.value.substring(1, startPos);
					offset += 1;
				}
			}
			me.value = beginStr +
			      me.value.substring(startPos, endPos).replace(/\n\t/g, "\n") +
			      me.value.substring(endPos, me.value.length);
			me.focus();
			me.selectionStart = startPos - offset;
			me.selectionEnd = endPos - count - offset;
		}
		me.scrollTop = scrollTop;
		offset = -1;
	});
};

function loadDoc(file) {
	if (window.XMLHttpRequest) xhttp=new XMLHttpRequest();
	else xhttp=new ActiveXObject("Microsoft.XMLHTTP");
	xhttp.open("GET",file,false);
	xhttp.send();
	return xhttp.responseText;
} 

function loadXMLDoc(file) {
	if (window.XMLHttpRequest) xhttp=new XMLHttpRequest();
	else xhttp=new ActiveXObject("Microsoft.XMLHTTP");
	xhttp.open("GET",file,false);
	xhttp.send();
	return xhttp.responseXML;
} 

function onKeyPress(key) {
	var k = $("#h"+key.attr("id"));
	k.find(".shape").attr("class", k.find(".shape").attr("class").replace("hover", "selected"));
	$('#input').focus();
};

function latchorlock(modifier, lorl) {
	$(".key."+modifier).find(".shape").each(function() {
		$(this).attr("class", lorl + " " + $(this).attr("class"));
	});
	globalmod |= parseInt(modifier.substring(3));
}

function unlatchorunlock(modifier, lorl) {
	$(".key."+modifier).find(".shape").each(function() {
		$(this).attr("class", $(this).attr("class").replace(lorl + " ", ""));
	});
	globalmod &= (255 - parseInt(modifier.substring(3)));
}

function updateGlobalMod(oldgmod) {
	if (oldgmod != globalmod) {
		$("text.mod" + globalmod + ",use.mod" + globalmod).each(function(){
			$(this).parent().find("text.symbol,use.symbol").hide();
			$(this).show();
		});
	}
}

function onKeyRelease(key) {
	var k = $("#h"+key.attr("id"));
	k.find(".shape").attr("class", k.find(".shape").attr("class").replace("selected", "hover"));
	var c = "";
	k.find("text.symbol,use.symbol").each(function() {
		if ((typeof $(this).attr("style") === "undefined") || ($(this).attr("style") != "display: none;")) {
			c = $(this).attr("class");
		}
	});
	if (c.search("BackSpace") > -1) {
		$("#input").backSpace();
	} else if (c.search("Return") > -1) {
		$("#input").insertAtCaret("\n");
	} else if (c.search("ISO_Left_Tab") > -1) {
		$("#input").leftTab();
	} else if (c.search("Tab") > -1) {
		$("#input").tab();
	} else if (c.search("Left") > -1) {
		$("#input").left();
	} else if (c.search("Right") > -1) {
		$("#input").right();
	} else if (c.search("Up") > -1) {
		$("#input").up();
	} else if (c.search("Down") > -1) {
		$("#input").down();
	} else {
		var t = "";
		k.find("text.symbol").each(function() {
			if ((typeof $(this).attr("style") === "undefined") || ($(this).attr("style") != "display: none;")) {
				t = $(this).text()
			}
		});
		if (t.length == 1) $("#input").insertAtCaret(t);
	}

	var modifier = key.attr("class");
	var oldgmod = globalmod;
	if (modifier[3] == " ") {
		modifier = modifier.split(" ")[1]
		if (key.attr("class").search("locker") > -1) {
			if (-1 == key.find(".shape").attr("class").search("locked")) {
				latchorlock(modifier, "locked");
			} else {
				unlatchorunlock(modifier, "locked");
			}
		} else if (key.find(".shape").attr("class").search("locked") > -1) {
			unlatchorunlock(modifier, "locked");
		} else if (key.find(".shape").attr("class").search("latched") > -1) {
			unlatchorunlock(modifier, "latched");
			latchorlock(modifier, "locked");
		} else {
			latchorlock(modifier, "latched");
		}
	} else {
		$(".latched").each(function() {
			modifier = $(this).parent().attr("class").split(" ")[1];
			unlatchorunlock(modifier, "latched");
		});
	}
	updateGlobalMod(oldgmod);

	$('#input').focus();
};

var hoverKey = 0;
function onMouseOver(key) {
	if (key == 0) {
		if (hoverKey != 0) {
			hoverKey.hide();
			hoverKey = 0;
		}
	} else {
		k = $("#h"+key.attr("id"));
		if (k != hoverKey) {
			if (hoverKey != 0) {
				hoverKey.hide();
			}
			hoverKey = k;
			k.show();
		}
	}
};

var doc = loadDoc("florence.svg");
var globalmod = "0";

$(document).ready(function() {
	$('#keyboard').html(doc);

	keyboardWidth = parseFloat($('#keyboard').find("svg").attr("width"));
	keyboardHeight = parseFloat($('#keyboard').find("svg").attr("height"));

	$('.key').each(function(){
		copy = $(this).clone();

		x = parseFloat(copy.attr('x'));
		y = parseFloat(copy.attr('y'));
		w = parseFloat(copy.attr('width'));
		h = parseFloat(copy.attr('height'));
		coords = $(this).parent().attr("transform").split("(")[1].split(")")[0].split(",")
		ox = parseFloat(coords[0]);
		oy = parseFloat(coords[1]);
		e = (-0.5*(x+(w/2.0))+ox);
		f = (-0.5*(y+(h/2.0))+oy);

		if ( (ox+x-(w/4.0)) < 0 ) e+= (w/4.0);
		else if ( (ox+x+(w*1.25)) > keyboardWidth ) e-= (w/4.0);
		if ( (oy+y-(h/4.0)) < 0 ) f+= (h/4.0);
		else if ( (oy+y+(h*1.25)) > keyboardHeight ) f-= (h/4.0);

		copy.attr("id", "h" + copy.attr("id"));
		copy.attr("transform", "matrix(1.5, 0, 0, 1.5, "+e.toString()+", "+f.toString()+")");
		copy.css("pointer-events", "none");
		copy.find(".shape").attr("class", copy.find(".shape").attr("class") + " hover");
		copy.hide();

		$(this).parent().parent().append(copy);

		$(this).mouseover(function(){onMouseOver($(this))});
		$(this).mouseout(function(){onMouseOver(0)});
		$(this).mousedown(function(){onKeyPress($(this))});
		$(this).mouseup(function(){onKeyRelease($(this))});
	});

});

