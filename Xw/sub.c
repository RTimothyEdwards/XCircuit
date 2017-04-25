/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        sub.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextEdit widget
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 **   Permission to use, copy, modify, and distribute this software 
 **   and its documentation for any purpose and without fee is hereby 
 **   granted, provided that the above copyright notice appear in all 
 **   copies and that both that copyright notice and this permission 
 **   notice appear in supporting documentation, and that the names of 
 **   Hewlett-Packard, Digital or  M.I.T.  not be used in advertising or 
 **   publicity pertaining to distribution of the software without 
 **   written prior permission.
 **   
 **   DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 **   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 **   DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 **   ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 **   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 **   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 **   SOFTWARE.
 **   
 *****************************************************************************
 *************************************<+>*************************************/
/*****************************************************************************
*
*  Procedures declared in this file
*
******************************************************************************
*/
static void           InsertCursor () ;
static void           _XtTextNeedsUpdating() ;
static XwTextPosition PositionForXY () ;
static int            LineForPosition () ;
static int            LineAndXYForPosition () ;
static void           BuildLineTable () ;
static XwTextLineRange UpdateLineTable () ;
static void           ForceBuildLineTable() ;
static void           _XtTextScroll() ;
static XwEditResult   ReplaceText () ;
static void           DisplayText() ;
static void           ClearWindow () ;
static void           ClearText () ;     
static void           DisplayAllText () ;
static void           CheckResizeOrOverflow() ;
static void           ProcessExposeRegion() ;
static void           _XtTextPrepareToUpdate() ;
static void           FlushUpdate() ;
static void           _XtTextShowPosition() ;
static void           _XtTextExecuteUpdate() ;

/*
 * Procedure to manage insert cursor visibility for editable text.  It uses
 * the value of ctx->insertPos and an implicit argument. In the event that
 * position is immediately preceded by an eol graphic, then the insert cursor
 * is displayed at the beginning of the next line.
 */
/*--------------------------------------------------------------------------+*/
static void InsertCursor (ctx, state)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwInsertState state;
{
    Position x, y;
    int dy, line, visible;
    XwTextBlock text;

    if (ctx->text.lt.lines < 1) return;
    visible = LineAndXYForPosition(ctx, ctx->text.insertPos, &line, &x, &y);
    if (line < ctx->text.lt.lines)
	dy = (ctx->text.lt.info[line + 1].y - ctx->text.lt.info[line].y) + 1;
    else
	dy = (ctx->text.lt.info[line].y - ctx->text.lt.info[line - 1].y) + 1;

    /** If the insert position is just after eol then put it on next line **/
    if (x > ctx->text.leftmargin &&
	ctx->text.insertPos > 0 &&
	ctx->text.insertPos >= GETLASTPOS(ctx)) {
	   /* reading the source is bogus and this code should use scan */
	   (*(ctx->text.source->read)) (ctx->text.source,
					ctx->text.insertPos - 1, &text, 1);
	   if (text.ptr[0] == '\n') {
	       x = ctx->text.leftmargin;
	       y += dy;
	   }
    }
    y += dy;
    if (visible)
	(*(ctx->text.sink->insertCursor))(ctx, x, y, state);
}


/*
 * Procedure to register a span of text that is no longer valid on the display
 * It is used to avoid a number of small, and potentially overlapping, screen
 * updates. [note: this is really a private procedure but is used in
 * multiple modules].
 */
/*--------------------------------------------------------------------------+*/
static void _XtTextNeedsUpdating(ctx, left, right)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition left, right;
{
    int     i;
    if (left < right) {
	for (i = 0; i < ctx->text.numranges; i++) {
	    if (left <= ctx->text.updateTo[i]
		&& right >= ctx->text.updateFrom[i])
	      { ctx->text.updateFrom[i] = min(left, ctx->text.updateFrom[i]);
		ctx->text.updateTo[i] = max(right, ctx->text.updateTo[i]);
		return;
	    }
	}
	ctx->text.numranges++;
	if (ctx->text.numranges > ctx->text.maxranges) {
	    ctx->text.maxranges = ctx->text.numranges;
	    i = ctx->text.maxranges * sizeof(XwTextPosition);
	    ctx->text.updateFrom = (XwTextPosition *) 
   	        XtRealloc((char *)ctx->text.updateFrom, (unsigned) i
		* sizeof(XwTextPosition));
	    ctx->text.updateTo = (XwTextPosition *) 
		XtRealloc((char *)ctx->text.updateTo, (unsigned) i
		* sizeof(XwTextPosition));
	}
	ctx->text.updateFrom[ctx->text.numranges - 1] = left;
	ctx->text.updateTo[ctx->text.numranges - 1] = right;
    }
}


/* 
 * This routine maps an x and y position in a window that is displaying text
 * into the corresponding position in the source.
 */
/*--------------------------------------------------------------------------+*/
static XwTextPosition PositionForXY (ctx, x, y)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  Position x,y;
{
 /* it is illegal to call this routine unless there is a valid line table! */
    int     width, fromx, line;
    XwTextPosition position, resultstart, resultend;
    XwTextPosition lastpos = GETLASTPOS(ctx);

    /*** figure out what line it is on ***/
    for (line = 0; line < ctx->text.lt.lines - 1; line++) {
	if (y <= ctx->text.lt.info[line + 1].y)
	    break;
    }
    position = ctx->text.lt.info[line].position;
    if (position >= lastpos)
	return lastpos;
    fromx = ctx->text.lt.info[line].x;	/* starting x in line */
    width = x - fromx;			/* num of pix from starting of line */
    (*(ctx->text.sink->resolve)) (ctx, position, fromx, width,
	    &resultstart, &resultend);
    if (resultstart >= ctx->text.lt.info[line + 1].position)
	resultstart = (*(ctx->text.source->scan))(ctx->text.source,
		ctx->text.lt.info[line + 1].position, XwstPositions, XwsdLeft,
						  1, TRUE);
    return resultstart;
}

/*
 * This routine maps a source position in to the corresponding line number
 * of the text that is displayed in the window.
 */
/*--------------------------------------------------------------------------+*/
static int LineForPosition (ctx, position)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition position;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    int     line;

    if (position <= ctx->text.lt.info[0].position)
	return 0;
    for (line = 0; line < ctx->text.lt.lines; line++)
	if (position < ctx->text.lt.info[line + 1].position)
	    break;
    return line;
}

/*
 * This routine maps a source position into the corresponding line number
 * and the x, y coordinates of the text that is displayed in the window.
 */
/*--------------------------------------------------------------------------+*/
static int LineAndXYForPosition (ctx, pos, line, x, y)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition pos;
  int *line;
  Position *x, *y;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    XwTextPosition linePos, endPos;
    int     visible, realW, realH;

    *line = 0;
    *x = ctx->text.leftmargin;
    *y = ctx->text.topmargin;
    visible = IsPositionVisible(ctx, pos);
    if (visible) {
	*line = LineForPosition(ctx, pos);
	*y = ctx->text.lt.info[*line].y;
	*x = ctx->text.lt.info[*line].x;
	linePos = ctx->text.lt.info[*line].position;
	(*(ctx->text.sink->findDistance))(ctx, linePos,
                                     *x, pos, &realW, &endPos, &realH);
	*x = *x + realW;
    }
    return visible;
}

/*
 * This routine builds a line table. It does this by starting at the
 * specified position and measuring text to determine the staring position
 * of each line to be displayed. It also determines and saves in the
 * linetable all the required metrics for displaying a given line (e.g.
 * x offset, y offset, line length, etc.).
 */
/*--------------------------------------------------------------------------+*/
static void BuildLineTable (self, position)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget self;
  XwTextPosition position;
{
    XwTextPosition line, lines;
    Boolean     rebuild;
    XwLineTableEntryPtr lp ;

    rebuild = (Boolean) (position != self->text.lt.top);
    lines= applyDisplay(maxLines)(self) ;

/****************
 *
 *  RBM 
 *
 *  Don't allow a 0-line widget - let clipping occur
 *
 *  NOTE: THE MAXINT CLIPPING IS AN UGLY HACK THAT NEEDS TO BE FIXED 
 *        WITH A CAST!!
 ****************/
    if ((lines < 1) || (lines > 32767)) lines = 1;

    if (self->text.lt.info != NULL && lines != self->text.lt.lines) {
	XtFree((char *) self->text.lt.info);
	self->text.lt.info = NULL;
    }
    if (self->text.lt.info == NULL)
      {	self->text.lt.info = (XwLineTableEntry *)
	  XtCalloc(lines + 1, (unsigned)sizeof(XwLineTableEntry));
	self->text.lt.lines = lines ;
	for (line = 0, lp = &(self->text.lt.info[0]) ;
	     line < lines; line++, lp++)
	  { lp->position = lp->drawPos = 0 ;
	    lp->x = lp->y = 0 ;
	  }
	rebuild = TRUE;
      }

    self->text.lt.top = position ;
    if (rebuild) UpdateLineTable ( self
				  , position
				  , 0
				  , self->core.width
				    - self->text.leftmargin
				    - self->text.rightmargin
				  , 0
				  , FALSE
				  ) ;
}

/*--------------------------------------------------------------------------+*/
static XwTextLineRange UpdateLineTable
  (self, pos0, posF, width, line0, updateMode)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget	self ;
     XwTextPosition	pos0, posF ;
     Dimension		width ;
     XwTextPosition	line0 ;
     int		updateMode ;
{
  XwLineTableEntryPtr currLine ;
  XwLineTablePtr      lt ;
  XwTextPosition line, nLines ;
  Dimension	x0, y ;
  int		reqW, reqH ;
  TextFit	(*textFitFn)();
  int		wrapEnabled, breakOnWhiteSpace ;
  XwTextPosition fitPos, drawPos, nextPos ;
  
  textFitFn =  self->text.sink->textFitFn ;
  lt = &(self->text.lt) ;
  nLines = lt->lines ;
  x0 = self->text.leftmargin ;
  y = updateMode ? lt->info[line0].y : self->text.topmargin ;
  reqH = 0 ;
  wrapEnabled = (int) self->text.wrap_mode ;
  breakOnWhiteSpace =
    wrapEnabled && (self->text.wrap_break == XwWrapWhiteSpace) ;
  
  for (line = line0; line <= nLines; line++)
    { currLine = &(lt->info[line]) ;
      currLine->x = x0;
      currLine->y = y;
      currLine->position = pos0;
      if (pos0 <= GETLASTPOS(self))
	{ currLine->fit = (*textFitFn) ( self
					, pos0
					, x0
					, width
					, wrapEnabled
					, breakOnWhiteSpace
					, &fitPos
					, &drawPos
					, &nextPos
					, &reqW
					, &reqH
					) ;
	  currLine->drawPos = drawPos ;

	  currLine->endX = x0 + reqW ;
	  pos0 = nextPos;
	  /* In update mode we must go through the last line which had a
	     character replaced in it before terminating on a mere position
	     match.  Starting position (of replacement) would be sufficient
	     only if we know the font is fixed width. (Good place to
	     optimize someday, huh?)
	     */
	  if (updateMode && (nextPos > posF)
	      && (nextPos == lt->info[line+1].position))
	    { break ;
	     }
	}
      else
	{ currLine->endX = x0;
	  currLine->fit = tfEndText ;
	}
      y += reqH;
    } ;

  return ( (line < nLines) ? line : nLines - 1 ) ;
}


/*
 * This routine is used to re-display the entire window, independent of
 * its current state.
 */
/*--------------------------------------------------------------------------+*/
static void ForceBuildLineTable(ctx)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget ctx;
{
    XwTextPosition position;

    position = ctx->text.lt.top;
    ctx->text.lt.top++; /* ugly, but it works */
    BuildLineTable(ctx, position);
}

/*
 * The routine will scroll the displayed text by lines.  If the arg  is
 * positive, move up; otherwise, move down. [note: this is really a private
 * procedure but is used in multiple modules].
 */
/*--------------------------------------------------------------------------+*/
static void _XtTextScroll(ctx, n)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  int n;
{
  register XwTextEditPart *text = (XwTextEditPart *) &(ctx->text);
  register XwLineTablePtr lt = &(text->lt);
  XwTextPosition top, target, lastpos = GETLASTPOS(ctx);
  Dimension textwidth =
                      ctx->core.width - (text->leftmargin + text->rightmargin);
  Dimension ypos;
    if (n >= 0) {
	top = min(lt->info[n].position, lastpos);
	BuildLineTable(ctx, top);
	if (top >= lastpos)
	    DisplayAllText(ctx);
	else {
	    XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx),
		      text->gc,
		      text->leftmargin,		      /* 0, */
		      lt->info[n].y,
		      textwidth,                     /* 9999, */
		      ctx->core.height - lt->info[n].y - text->bottommargin,
		      text->leftmargin,              /* 0, */
		      lt->info[0].y);
	    ypos = lt->info[0].y + ctx->core.height - lt->info[n].y;
	    (*(text->sink->clearToBackground))
	      (ctx,
	       text->leftmargin,                    /* 0, */
	       ypos,      /* lt->info[0].y + ctx->core.height - lt->info[n].y, */
	       textwidth,                           /* 9999, */
	       ctx->core.height - (ypos + text->bottommargin)    /* 9999 */
	       );
	    if (n < lt->lines) n++;
	    _XtTextNeedsUpdating(ctx,
		    lt->info[lt->lines - n].position, lastpos);
	}
    } else {
	Dimension tempHeight;
	n = -n;
	target = lt->top;
	top = (*(text->source->scan))(text->source, target, XwstEOL,
				     XwsdLeft, n+1, FALSE);
	tempHeight = lt->info[lt->lines-n].y - text->topmargin;
	BuildLineTable(ctx, top);
	if (lt->info[n].position == target) {
	    XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx),
		      text->gc,
		      text->leftmargin,            /* 0, */
		      lt->info[0].y,
		      textwidth,                   /* 9999, */
		      tempHeight,
		      text->leftmargin,            /* 0, */
		      lt->info[n].y);
	    _XtTextNeedsUpdating(ctx, 
		    lt->info[0].position, lt->info[n].position);
	} else if (lt->top != target) DisplayAllText(ctx);
    }
}


/*
 * This internal routine deletes the text from pos1 to pos2 in a source and
 * then inserts, at pos1, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any).
 */
/*--------------------------------------------------------------------------+*/
static XwEditResult ReplaceText (ctx, pos1, pos2, text, verify)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition pos1, pos2;
  XwTextBlock *text;
  Boolean     verify;

 /* it is illegal to call this routine unless there is a valid line table!*/
{
    int             i, line1, line2, visible, delta;
    XwEditResult    error;
    Position        x, y;
    XwTextPosition  startPos, endPos, updateFrom, lastpos;
    XwTextVerifyCD  cbdata;
    XwTextBlock	    newtxtblk;

    newtxtblk.ptr = (unsigned char*) XtMalloc(text->length);
    newtxtblk.firstPos = text->firstPos;
    newtxtblk.length = text->length;
    strncpy(newtxtblk.ptr, text->ptr, text->length);
    cbdata.operation = modVerify;
    cbdata.doit = TRUE;
    cbdata.currInsert = ctx->text.insertPos;
    cbdata.newInsert = ctx->text.insertPos;
    cbdata.startPos = pos1;
    cbdata.endPos = pos2;
    cbdata.text = &newtxtblk;

    if (verify)
      { XtCallCallbacks((Widget)ctx, XtNmodifyVerification, &cbdata);

	if (!cbdata.doit) {
	  text->length = 0;  /* Necessary inorder to return to initial state */
	  return XweditReject;
	}
	
	/* Extract any new data changed by the verification callback */
	/* newtxtblk is used in the actual replace call later */
	pos1 = cbdata.startPos;
	pos2 = cbdata.endPos;
	ctx->text.insertPos = cbdata.newInsert;
      }
    
    /* the insertPos may not always be set to the right spot in XwtextAppend */
    if ((pos1 == ctx->text.insertPos) && 
        ((*(ctx->text.source->editType))(ctx->text.source) == XwtextAppend)) {
      ctx->text.insertPos = GETLASTPOS(ctx);
      pos2 = pos2 - pos1 + ctx->text.insertPos;
      pos1 = ctx->text.insertPos;
    }
    updateFrom = (*(ctx->text.source->scan))
      (ctx->text.source, pos1, XwstWhiteSpace, XwsdLeft, 1, TRUE);
    updateFrom = (*(ctx->text.source->scan))
      (ctx->text.source, updateFrom, XwstPositions, XwsdLeft, 1, TRUE);
    startPos = max(updateFrom, ctx->text.lt.top);
    visible = LineAndXYForPosition(ctx, startPos, &line1, &x, &y);
    error = (*(ctx->text.source->replace))
      (ctx->text.source, pos1, pos2, &newtxtblk, &delta);
    XtFree(newtxtblk.ptr);
    if (error) return error;
    lastpos = GETLASTPOS(ctx);
    if (ctx->text.lt.top >= lastpos) {
	BuildLineTable(ctx, lastpos);
	/* ClearWindow(ctx); */
	ClearText(ctx); 	
	return error;
    }
    if (delta < lastpos) {
	for (i = 0; i < ctx->text.numranges; i++) {
	    if (ctx->text.updateFrom[i] > pos1)
		ctx->text.updateFrom[i] += delta;
	    if (ctx->text.updateTo[i] >= pos1)
		ctx->text.updateTo[i] += delta;
	}
    }

    line2 = LineForPosition(ctx, pos1);
    /* 
     * fixup all current line table entries to reflect edit.
     * BUG: it is illegal to do arithmetic on positions. This code should
     * either use scan or the source needs to provide a function for doing
     * position arithmetic.
    */
    for (i = line2 + 1; i <= ctx->text.lt.lines; i++)
	ctx->text.lt.info[i].position += delta;

    endPos = pos1;
    /*
     * Now process the line table and fixup in case edits caused
     * changes in line breaks. If we are breaking on word boundaries,
     * this code checks for moving words to and from lines.
    */
    if (visible) {
      XwTextLineRange lastChangedLine ;
      if (line1) line1-- ;	/* force check for word moving to prev line */
      lastChangedLine =
	UpdateLineTable (ctx
			 , ctx->text.lt.info[line1].position
			 , pos2 + delta
			 , ctx->core.width - ctx->text.leftmargin
			   - ctx->text.rightmargin
			 , line1
			 , TRUE
			 ) ;
      endPos = ctx->text.lt.info[lastChangedLine+1].position ;
    }
    lastpos = GETLASTPOS(ctx);
    if (delta >= lastpos)
	endPos = lastpos;
    if (delta >= lastpos || pos2 >= ctx->text.lt.top)
	_XtTextNeedsUpdating(ctx, updateFrom, endPos);
    return error;
}


/*
 * This routine will display text between two arbitrary source positions.
 * In the event that this span contains highlighted text for the selection, 
 * only that portion will be displayed highlighted.
 */
/*--------------------------------------------------------------------------+*/
static void DisplayText(ctx, pos1, pos2)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition pos1, pos2;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    Position x, y, xlimit, tempx;
    Dimension height;
    int line, i, visible;
    XwTextPosition startPos, endPos, lastpos = GETLASTPOS(ctx);

    if (pos1 < ctx->text.lt.top)
	pos1 = ctx->text.lt.top;
    if (pos2 > lastpos) 
	pos2 = lastpos;
    if (pos1 >= pos2) return;
    visible = LineAndXYForPosition(ctx, pos1, &line, &x, &y);
    if (!visible)
	return;
    startPos = pos1;
    xlimit = ctx->core.width - ctx->text.rightmargin + 1 ;
    height = ctx->text.lt.info[1].y - ctx->text.lt.info[0].y;
    for (i = line; i < ctx->text.lt.lines; i++) {
	endPos = ctx->text.lt.info[i].drawPos + 1;
	if (endPos > pos2)
	    endPos = pos2;
	if (endPos > startPos) {
	  /*  We know this should not be necessary!!!!
	    if (x == ctx->text.leftmargin)
                (*(ctx->text.sink->clearToBackground))(ctx,
	             0, y, ctx->text.leftmargin, height);
          */
	    if (startPos >= ctx->text.s.right || endPos <= ctx->text.s.left) {
		(*(ctx->text.sink->display))(ctx, x, y,
			startPos, endPos, FALSE);
	    } else if (startPos >= ctx->text.s.left
		       && endPos <= ctx->text.s.right)
	      { (*(ctx->text.sink->display))(ctx, x, y,
			startPos, endPos, TRUE);
	    } else {
		DisplayText(ctx, startPos, ctx->text.s.left);
		DisplayText(ctx, max(startPos, ctx->text.s.left), 
			min(endPos, ctx->text.s.right));
		DisplayText(ctx, ctx->text.s.right, endPos);
	    }
	}
	startPos = ctx->text.lt.info[i + 1].position;
	height = ctx->text.lt.info[i + 1].y - ctx->text.lt.info[i].y;
	tempx = ctx->text.lt.info[i].endX;
        (*(ctx->text.sink->clearToBackground))(ctx,
	    tempx, y, xlimit - tempx, height);
	x = ctx->text.leftmargin;
	y = ctx->text.lt.info[i + 1].y;
	if ((endPos == pos2) && (endPos != lastpos))
	    break;
    }
}

/*
 * Clear the window to background color.
 */
/*--------------------------------------------------------------------------+*/
static void ClearWindow (ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
    (*(ctx->text.sink->clearToBackground))(ctx, 0, 0, ctx->core.width,
					   ctx->core.height);
}

/*
 * Clear the portion of the window that the text in drawn in, or that is
 * don't clear the margins  
 */
/*--------------------------------------------------------------------------+*/
static void ClearText (ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
  register XwTextEditPart *text = (XwTextEditPart *) &(ctx->text);
  (*(text->sink->clearToBackground))
             (ctx, 0, 0, ctx->core.width, ctx->core.height);
}

/*
 * Internal redisplay entire window.
 */
/*--------------------------------------------------------------------------+*/
static void DisplayAllText (w)
/*--------------------------------------------------------------------------+*/
  Widget w;
{
    XwTextEditWidget ctx = (XwTextEditWidget) w;

    if (!XtIsRealized((Widget)ctx)) return;

    ClearText(ctx);
    /* ClearWindow(ctx); */
    /* BuildLineTable(ctx, ctx->text.lt.top); */
    _XtTextNeedsUpdating(ctx, zeroPosition, GETLASTPOS(ctx));
 
}

/*
 * This routine checks to see if the window should be resized (grown or
 * shrunk) or scrolled then text to be painted overflows to the right or
 * the bottom of the window. It is used by the keyboard input routine.
 */
/*--------------------------------------------------------------------------+*/
static void CheckResizeOrOverflow(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
  XwTextLineRange i, nLines ;
  Dimension width;
  XtWidgetGeometry rbox;
  XtGeometryResult reply;
  XwLineTableEntryPtr lp ;
  XwLineTablePtr      lt ;
  XwTextEditPart   *tp ;

  tp = &(ctx->text) ;
  lt = &(tp->lt) ;
  nLines = lt->lines ;
  
  if (tp->grow_state & XwGrowHorizontal)
    { UpdateLineTable ( ctx, lt->top, 0, INFINITE_WIDTH, 0, FALSE) ;
      width = 0 ;
      for (i=0, lp = &(lt->info[0]) ; i < nLines ; i++, lp++)
	{ if (width < lp->endX) width = lp->endX ;
	} ;
      width += ctx->text.rightmargin;
      if (width > ctx->core.width)
	{ rbox.request_mode = CWWidth;
	  rbox.width = width;
	  reply = XtMakeGeometryRequest((Widget)ctx, &rbox, &rbox);
	  if (reply == XtGeometryAlmost)
	    reply = XtMakeGeometryRequest((Widget)ctx, &rbox, NULL);
	  /* NOTE: following test is expected to be a fall-through from
	     previous.  Should not be an else if. */
	  if (reply == XtGeometryYes)
	    ctx->core.width = rbox.width;
	  else	/* if request not satisfied, disallow future attempts */
	    { tp->grow_state &= ~XwGrowHorizontal ;
	      UpdateLineTable ( ctx, lt->top, 0, ctx->core.width
			- ctx->text.leftmargin - ctx->text.rightmargin,
			0, FALSE);
	    }
	}
    } ;
  
  if ((tp->grow_state & XwGrowVertical)
      && ( ! (lt->info[nLines].fit & tfEndText)
	  || (lt->info[nLines].drawPos > lt->info[nLines].position))
      )
    { rbox.request_mode = CWHeight;
      rbox.height = (*(ctx->text.sink->maxHeight))
	(ctx, nLines + 1) + tp->topmargin + tp->bottommargin ;
      reply = XtMakeGeometryRequest((Widget)ctx, &rbox, &rbox);
      if (reply == XtGeometryAlmost)
	reply = XtMakeGeometryRequest((Widget)ctx, &rbox, NULL);
      if (reply == XtGeometryYes)
	ctx->core.height = rbox.height;
      else	/* if request not satisfied, disallow future attempts */
	{ tp->grow_state &= ~XwGrowVertical ;
	}
    } ;
}

/*
 * This routine processes all "expose region" XEvents. In general, its job
 * is to the best job at minimal re-paint of the text, displayed in the
 * window, that it can.
 */
/*--------------------------------------------------------------------------+*/
static void ProcessExposeRegion(w, event)
/*--------------------------------------------------------------------------+*/
  Widget w;
  XEvent *event;
{
    XwTextEditWidget ctx = (XwTextEditWidget) w;
    XwTextPosition pos1, pos2, resultend;
    int line;
    int x = event->xexpose.x;
    int y = event->xexpose.y;
    int width = event->xexpose.width;
    int height = event->xexpose.height;
    XwLineTableEntryPtr info;

   _XtTextPrepareToUpdate(ctx);
    if (x < ctx->text.leftmargin) /* stomp on caret tracks */
        (*(ctx->text.sink->clearToBackground))(ctx, x, y, width, height);
   /* figure out starting line that was exposed */
    line = LineForPosition(ctx, PositionForXY(ctx, x, y));
    while (line < ctx->text.lt.lines && ctx->text.lt.info[line + 1].y < y)
	line++;
    while (line < ctx->text.lt.lines) {
	info = &(ctx->text.lt.info[line]);
	if (info->y >= y + height)
	    break;
	(*(ctx->text.sink->resolve))(ctx, 
                                info->position, info->x,
			        x - info->x, &pos1, &resultend);
	(*(ctx->text.sink->resolve))(ctx, 
                                info->position, info->x,
			        x + width - info->x, &pos2, 
                                &resultend);
	pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos2,
					   XwstPositions, XwsdRight, 1, TRUE);
	_XtTextNeedsUpdating(ctx, pos1, pos2);
	line++;
    }
    _XtTextExecuteUpdate(ctx);

}

/*
 * This routine does all setup required to syncronize batched screen updates
 */
/*--------------------------------------------------------------------------+*/
static void _XtTextPrepareToUpdate(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{

   if ((ctx->text.oldinsert < 0) && ctx->text.update_flag) {
	InsertCursor(ctx, XwisOff);
	ctx->text.numranges = 0;
	ctx->text.showposition = FALSE;
	ctx->text.oldinsert = ctx->text.insertPos;
    }
}


/*
 * This is a private utility routine used by _XtTextExecuteUpdate. It
 * processes all the outstanding update requests and merges update
 * ranges where possible.
 */
/*--------------------------------------------------------------------------+*/
static void FlushUpdate(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
    int     i, w;
    XwTextPosition updateFrom, updateTo;
    while (ctx->text.numranges > 0) {
	updateFrom = ctx->text.updateFrom[0];
	w = 0;
	for (i=1 ; i<ctx->text.numranges ; i++) {
	    if (ctx->text.updateFrom[i] < updateFrom) {
		updateFrom = ctx->text.updateFrom[i];
		w = i;
	    }
	}
	updateTo = ctx->text.updateTo[w];
	ctx->text.numranges--;
	ctx->text.updateFrom[w] = ctx->text.updateFrom[ctx->text.numranges];
	ctx->text.updateTo[w] = ctx->text.updateTo[ctx->text.numranges];
	for (i=ctx->text.numranges-1 ; i>=0 ; i--) {
	    while (ctx->text.updateFrom[i] <= updateTo
		   && i < ctx->text.numranges)
	      {
		updateTo = ctx->text.updateTo[i];
		ctx->text.numranges--;
		ctx->text.updateFrom[i] =
		  ctx->text.updateFrom[ctx->text.numranges];
		ctx->text.updateTo[i] =
		  ctx->text.updateTo[ctx->text.numranges];
	    }
	}
	DisplayText(ctx, updateFrom, updateTo);
    }
}


/*
 * This is a private utility routine used by _XtTextExecuteUpdate. This routine
 * worries about edits causing new data or the insertion point becoming
 * invisible (off the screen). Currently it always makes it visible by
 * scrolling. It probably needs generalization to allow more options.
 */
/*--------------------------------------------------------------------------+*/
static void _XtTextShowPosition(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
    XwTextPosition top, first, second, insertPos ;
    XwTextPosition lastpos = GETLASTPOS(ctx);
    XwTextEditPart *text = &(ctx->text) ;
    XwLineTablePtr lt = &(text->lt) ;
    short hScroll ;

    /* NOTE: Following code relies on current assumption that
       horizontal scrolling will be enabled only when there is
       only one display line.
       */

    insertPos = text->insertPos ;
    if (   insertPos < lt->top
	|| insertPos >= lt->info[lt->lines].position
	|| (hScroll = ((text->scroll_state & XwAutoScrollHorizontal)
		       && (insertPos > lt->info[0].drawPos)
		       && ( lt->info[0].drawPos + 1 < lastpos))
	    ? 1 : 0 )
	
	)
      {
	if (   lt->lines > 0
	    && (insertPos < lt->top 
		|| lt->info[lt->lines].position <=  lastpos
		|| hScroll)
	    ) {
	    first = lt->top;
	    second = lt->info[1].position;
/*
	    if (insertPos < first)
		top = (*(text->source->scan))(
			text->source, insertPos, XwstEOL,
			XwsdLeft, 1, FALSE);
	    else
		top = (*(text->source->scan))(
			text->source, insertPos, XwstEOL,
			XwsdLeft, lt->lines, FALSE);
	    BuildLineTable(ctx, top);
	    while (insertPos >= lt->info[lt->lines].position) {
		if (lt->info[lt->lines].position > lastpos)
		    break;
		BuildLineTable(ctx, lt->info[1].position);
	    }
*/
	    if (text->scroll_state & XwAutoScrollHorizontal) {
		if ((insertPos > lt->info[0].drawPos) &&
		    (lt->info[0].drawPos + 1 < lastpos)) {
	           /* smooth scroll:  scroll by one character at a time */

		   XwTextPosition delta = 0 ;
		   top = insertPos - (lt->info[0].drawPos
				       - lt->info[0].position) - 1;
		   while (insertPos > lt->info[0].drawPos+1) {
		      BuildLineTable (ctx, top += delta) ;
		      delta = (insertPos - top) >> 2 ;
		   }
		   first = -1 ; /* prevent scroll down by one line */
	        }

		else if (insertPos < first) {
	           /* Do the same as above, for traveling to the left */

		   XwTextPosition delta = 0 ;
		   if (insertPos < 0) insertPos = 0;
		   top = insertPos;
		   if (top < 0) top = 0;
		   BuildLineTable (ctx, top);
		   while (insertPos < lt->top) {
		      top -= delta;
		      if (top < 0) top = 0;
		      BuildLineTable (ctx, top);
		      delta = (insertPos - top) >> 2;
		   }
		   first = -1 ; /* prevent scroll down by one line */
	        }
	    }

	    if (lt->top == second && lt->lines > 1) {
	        BuildLineTable(ctx, first);
		_XtTextScroll(ctx, 1);
	    } else if (lt->info[1].position == first && lt->lines > 1) {
		BuildLineTable(ctx, first);
		_XtTextScroll(ctx, -1);
	    } else {
		text->numranges = 0;
		if (lt->top != first)
		    DisplayAllText(ctx);
	    }
	}
    }
}



/*
 * This routine causes all batched screen updates to be performed
 */
/*--------------------------------------------------------------------------+*/
static void _XtTextExecuteUpdate(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
    if ((ctx->text.oldinsert >= 0) && ctx->text.update_flag) {
      if (ctx->text.oldinsert != ctx->text.insertPos
	  || ctx->text.showposition)
	_XtTextShowPosition(ctx);
      FlushUpdate(ctx);
      InsertCursor(ctx, XwisOn);
      ctx->text.oldinsert = -1;
    }
}
