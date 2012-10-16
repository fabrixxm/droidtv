/*****************************************************************************
*  DroidTV, live TV on Android devices with host USB port and a DVB tuner    *
*  Copyright (C) 2012  Christian Ulrich <chrulri@gmail.com>                  *
*                                                                            *
*  This program is free software: you can redistribute it and/or modify      *
*  it under the terms of the GNU General Public License as published by      *
*  the Free Software Foundation, either version 3 of the License, or         *
*  (at your option) any later version.                                       *
*                                                                            *
*  This program is distributed in the hope that it will be useful,           *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*  GNU General Public License for more details.                              *
*                                                                            *
*  You should have received a copy of the GNU General Public License         *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
*****************************************************************************/

package com.chrulri.droidtv;

import android.widget.VideoView;
import android.content.Context;
import android.util.AttributeSet;

public class CustomVideoView extends VideoView
{
	protected int _overrideWidth = 640;
	protected int _overrideHeight = 480;
	protected Boolean _useDefault = true;
		
	public CustomVideoView(Context c){
		super(c);
	}
	
	public CustomVideoView(Context c, AttributeSet s){
		super(c,s);
	}
	
	public void resizeVideo(int w, int h){
		_overrideWidth = w;
		_overrideHeight = h;
		_useDefault = false;
		
		getHolder().setFixedSize(w,h);
		
		requestLayout();
		invalidate();
	}
	
	public void defaultVideoSize(){
		_useDefault = true;
	}
	
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec){
		if (_useDefault) {
			setMeasuredDimension(widthMeasureSpec, heightMeasureSpec);
		} else {
			setMeasuredDimension(_overrideWidth, _overrideHeight);	
		}
	}
}
