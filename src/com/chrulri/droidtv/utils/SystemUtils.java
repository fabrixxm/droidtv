/******************************************************************************
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
 ******************************************************************************/

package com.chrulri.droidtv.utils;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;

import android.util.Log;

public final class SystemUtils {
	static final String TAG = SystemUtils.class.getName();

	public static final int ANDROID_CPU_ARM_FEATURE_ARMv7 = (1 << 0);
	public static final int ANDROID_CPU_ARM_FEATURE_VFPv3 = (1 << 1);
	public static final int ANDROID_CPU_ARM_FEATURE_NEON = (1 << 2);

	private static final String PROC_CPUINFO = "/proc/cpuinfo";
	private static final String PROC_CPUINFO_PROCESSOR = "Processor";
	private static final String PROC_CPUINFO_FEATURES = "Features";

	public static int getCpuFeatures() {
		try {
			int cpuFeatures = 0;
			final BufferedReader reader = new BufferedReader(
					new InputStreamReader(new FileInputStream(PROC_CPUINFO)));

			String line;
			while ((line = reader.readLine()) != null) {
				int i = line.indexOf(':');
				if (i < 0) {
					continue;
				}
				String key = line.substring(0, i).trim();
				String value = line.substring(i).trim();
				if (key.equalsIgnoreCase(PROC_CPUINFO_PROCESSOR)) {
					if (value.toLowerCase().endsWith("(v7l)")) {
						cpuFeatures |= ANDROID_CPU_ARM_FEATURE_ARMv7;
					}
				} else if (key.equalsIgnoreCase(PROC_CPUINFO_FEATURES)) {
					String[] features = value.split("\\s+");
					for (String feature : features) {
						if (feature.equalsIgnoreCase("vfpv3")) {
							cpuFeatures |= ANDROID_CPU_ARM_FEATURE_VFPv3;
						} else if (feature.equalsIgnoreCase("neon")) {
							cpuFeatures |= ANDROID_CPU_ARM_FEATURE_VFPv3
									| ANDROID_CPU_ARM_FEATURE_NEON;
						}
					}
				}
			}

			return cpuFeatures;
		} catch (Throwable t) {
			Log.e(TAG, PROC_CPUINFO, t);
			return 0;
		}
	}
}
