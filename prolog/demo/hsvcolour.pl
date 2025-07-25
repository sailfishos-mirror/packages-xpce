/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi-prolog.org
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  2001-2025, University of Amsterdam
			    SWI-Prolog Solutions b.v.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

:- module(pce_hsv_browser,
          [ hsv_browser/0
          ]).
:- use_module(library(pce)).

hsv_browser :-
    send(new(hsv_browser), open).

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module demonstrates the relation between the RBG colour-model used
internally and the HSV model.  The latter is often much more natural to
the user.  See colour->initialise for details.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

:- pce_begin_class(hsv_browser, dialog, "Browse colours in HSV model").

variable(current_colour, colour, get, "Current colour value").

item('H', hue,         0-360).
item('S', saturnation, 0-100).
item('V', value,       0-100).
item('R', red,         0-255).
item('G', green,       0-255).
item('B', blue,        0-255).

initialise(D, Init:[colour]) :->
    send_super(D, initialise, 'Demonstrate HSV'),
    send(D, append, new(hsv_candidate(hex, ' Exact: '))),
    send(D, append, new(hsv_candidate(named_1, ' Named 1: '))),
    send(D, append, new(hsv_candidate(named_2, ' Named 2: '))),
    send(D, append, new(hsv_candidate(named_3, ' Named 3: '))),
    (   item(Label, Selector, Low-High),
        send(D, append,
             new(Slider, slider(Label, Low, High, Low,
                                message(D, Selector, @arg1)))),
        send(Slider, drag, @on),
        send(Slider, attribute, hor_stretch, 100),
        send(Slider, width, 300),
        fail
    ;   true
    ),
    send(D, append, button(close)),
    send(D, resize_message, message(D, layout, @arg2)),
    (   Init \== @default
    ->  send(D, current_colour, Init)
    ;   send(D, current_colour, @display?foreground)
    ).

close(D) :->
    "Close the demo tool"::
    send(D, destroy).

:- pce_group(update).

current_colour(D, C:colour, From:[{rgb,hsv}]) :->
    "Set the current colour, updating all items"::
    send(D, slot, current_colour, C),
    (   From \== hsv
    ->  update(D, 'H', C, hue),
        update(D, 'S', C, saturnation),
        update(D, 'V', C, value)
    ;   true
    ),
    (   From \== rgb
    ->  update_rgb(D, 'R', C, red),
        update_rgb(D, 'G', C, green),
        update_rgb(D, 'B', C, blue)
    ;   true
    ),
    send(D, show, hex, C),
    send(D, show_named, C).

show(D, As:name, C:colour) :->
    "Show colour in item named As"::
    get(D, member, As, Item),
    send(Item, value, C).

show_named(D, C:colour) :->
    "Show close named colour"::
    closest_named_colour(C, 3, Closest),
    pairs_values(Closest, [C1,C2,C3]),
    send(D, show, named_1, C1),
    send(D, show, named_2, C2),
    send(D, show, named_3, C3).

update(D, Name, Colour, Selector) :-
    get(Colour, Selector, Value),
    get(D, member, Name, Item),
    send(Item, selection, Value).

update_rgb(D, Name, Colour, Selector) :-
    get(Colour, Selector, RGB),
    get(D, member, Name, Item),
    send(Item, selection, RGB).

value(D, Selector:name, Val) :<-
    "Get value from corresponding item"::
    item(ItemName, Selector, _),
    get(D, member, ItemName, Item),
    get(Item, selection, Val).

:- pce_group(hsv).

hue(D, H:'0..360') :->
    H2 is min(H, 359),
    get(D, value, saturnation, S),
    get(D, value, value, V),
    send(D, current_colour, colour(@default, H2, S, V, model := hsv), hsv).

saturnation(D, S:'0..100') :->
    get(D, value, hue, H),
    get(D, value, value, V),
    send(D, current_colour, colour(@default, H, S, V, model := hsv), hsv).

value(D, V:'0..360') :->
    get(D, value, hue, H),
    get(D, value, saturnation, S),
    send(D, current_colour, colour(@default, H, S, V, model := hsv), hsv).

:- pce_group(rgb).

red(D, R:'0..255') :->
    get(D, value, green, G),
    get(D, value, blue, B),
    rgb(R, G, B, Colour),
    send(D, current_colour, Colour, rgb).

green(D, G:'0..255') :->
    get(D, value, red, R),
    get(D, value, blue, B),
    rgb(R, G, B, Colour),
    send(D, current_colour, Colour, rgb).

blue(D, B:'0..255') :->
    get(D, value, red, R),
    get(D, value, green, G),
    rgb(R, G, B, Colour),
    send(D, current_colour, Colour, rgb).

rgb(R,G,B, colour(@default, R, G, B)).

:- pce_end_class(hsv_browser).

:- pce_begin_class(hsv_candidate, dialog_group).

initialise(Candidate, Name:name, Label:name) :->
    send_super(Candidate, initialise, Name, group),
    send(Candidate, append, new(Btn, button(copy))),
    send(Btn, label, image('16x16/copy.png')),
    send(Candidate, append, new(Nme, text('#xxxxxx')), right),
    send(Candidate, append, new(Txt,  text(Label)), right),
    send(Candidate, append, new(Box,  box(100, 20)), right),
    send(Box, alignment, right), % attribute, hor_stretch, 100),
    send(Btn, alignment, left),
    send(Nme, alignment, left),
    send(Nme, name, colour_name),
    send(Txt, alignment, right),
    send(Txt, name, label),
    send(Txt, background, @display?background),
    send(Candidate, attribute, hor_stretch, 100).

value(Candidate, Value:colour) :->
    "Set the selected colour"::
    get(Candidate, member, box, Box),
    send(Box, fill_pattern, Value),
    get(Candidate, member, colour_name, Text1),
    send(Text1, string, Value?name),
    get(Candidate, member, label, Text2),
    send(Text2, colour, Value).

copy(Candidate) :->
    "Copy current selection as colour name"::
    get(Candidate, member, colour_name, Text),
    send(@display, copy, Text?string).

:- pce_end_class(hsv_candidate).

:- dynamic
    colour_distance_to/2.


closest_named_colour(From, Top, Closest) :-
    retractall(colour_distance_to(_,_)),
    send(@colour_names, for_all,
         message(@prolog, distance, From, @arg1, @arg2)),
    findall(D-C, retract(colour_distance_to(C,D)), Pairs),
    sort(1, =<, Pairs, Sorted),
    length(Closest, Top),
    append(Closest, _, Sorted).

distance(Colour, To, RGB) :-
    get(Colour, distance, RGB, D),
    asserta(colour_distance_to(To, D)).
