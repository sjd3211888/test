// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/* $Id$   *
 * (c) Copyright, eProsima, 2009.                                          *
 * All rights reserved.                                                    *
 *                                                                         *
 * No duplications, whole or partial, manual or electronic, may be made    *
 * without express written permission.  Any such copies, or                *
 * revisions thereof, must display this notice unaltered.                  *
 * This code contains trade secrets of                                     *
 * eProsima (Proyectos y Sistemas de Mantenimiento S.L.)                   *
 *                                                                         *
 * modification history                                                    *
 * --------------------                                                    *
 * 1.0,29sep09,RodM Created                                                *
 * =====================================================================   *
 */

package com.eprosima.vbsrpc.util;

public class VSConfiguration {

	private String name;
	private String platform;
	private boolean debug;
	private boolean dll;

	public VSConfiguration(){
		name = null;
		platform = null;
		debug = false;
		dll = false;
	}

	public VSConfiguration(String name, String platform, boolean debug, boolean dll){
		this.name = name;
		this.platform = platform;
		this.debug = debug;
		this.dll = dll;
	}

	public String getName(){
		return name;
	}

	public String getPlatform()
	{
		return platform;
	}

	public void setPlatform(String platform)
	{
		this.platform = platform;
	}

	public boolean isDll()
	{
		return dll;
	}

	public boolean isDebug()
	{
		return debug;
	}

	public boolean isRelease()
	{
		return !debug;
	}
}
