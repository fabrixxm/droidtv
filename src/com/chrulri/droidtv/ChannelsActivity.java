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

package com.chrulri.droidtv;

import static com.chrulri.droidtv.utils.StringUtils.NEWLINE;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.widget.TextView;
import 	android.widget.Button;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.LinearLayout;
import android.widget.VideoView;
import android.view.MotionEvent;

import com.chrulri.droidtv.utils.ErrorUtils;
import com.chrulri.droidtv.utils.ParallelTask;
import com.chrulri.droidtv.utils.PreferenceUtils;
import com.chrulri.droidtv.utils.ProcessUtils;
import com.chrulri.droidtv.utils.StringUtils;
import com.chrulri.droidtv.utils.Utils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

public class ChannelsActivity extends Activity {
    private static final String TAG = ChannelsActivity.class.getSimpleName();

    private Spinner mSpinner;
    private ListView mListView;
    private String[] mChannelConfigs;
    private String[] mChannels;

    // stream relative
    static final int DVBLAST = R.raw.dvblast_2_1_0;
    static final int DVBLASTCTL = R.raw.dvblastctl_2_1_0;

    public static final String EXTRA_CHANNELCONFIG = "channelconfig";

    public enum DvbType {
        ATSC, DVBT, DVBC, DVBS
    }
    
    public class FrontendStatus {
        public static final int HAS_SIGNAL = 0x001;
        public static final int HAS_CARRIER = 0x02;
        public static final int HAS_VITERBI = 0x04;
        public static final int HAS_SYNC = 0x08;
        public static final int HAS_LOCK = 0x0F;
        public static final int REINIT = 0x10;

        public static final int SIGNAL_MAXVALUE = 0xFFFF;

        public int status;
        public long ber;
        public int signal;
        public int snr;

        @Override
        public String toString() {
            return String.format(
                    "FrontendStatus[status=%X, ber=%X, signal=%X, snr=%X]",
                    status, ber, signal, snr);
        }
    }

    static final String[] ENVP_TMPDIR = {
            "TMPDIR=."
    };

    static final String UDP_IP = "127.0.0.1";
    static final int UDP_PORT = 1555;

    // static final String HTTP_IP = "0.0.0.0"; // debug
    static final String HTTP_IP = "127.0.0.1";
    static final int HTTP_PORT = 1666;
    static final String HTTP_HEADER = "HTTP/1.1 200 OK" + NEWLINE +
            "Content-Type: video/mp2t" + NEWLINE +
            "Connection: keep-alive" + NEWLINE + NEWLINE;

    public static final String SERVICE_URL = String.format("http://127.0.0.1:%d/tv.ts",
            HTTP_PORT);

    /**
     * ip:port 1 serviceid
     */
    static final String DVBLAST_CONFIG_CONTENT = UDP_IP + ":" + UDP_PORT + " 1 %d";
    static final String DVBLAST_CONFIG_FILENAME = "dvblast.conf";
    static final String DVBLAST_SOCKET = "droidtv.socket";
    static final String DVBLAST_LOG = "dvblast.log";
    static final int DVBLAST_CHECKDELAY = 500;

    private FrontendStatus mFrontendStatus;
    private String mChannelName;
    private String mChannelConfig;
    private final Handler mHandler = new Handler();
    private AsyncDvblastTask mDvblastTask;
    private AsyncDvblastCtlTask mDvblastCtlTask;
    private AsyncStreamTask mStreamTask;
    private DatagramSocket mUdpSocket;
    private ServerSocket mHttpSocket;
    private VideoView mVideoView;
    private LinearLayout mChanList;
    private LinearLayout mControlButs;
    //private boolean chanToggle;
    private boolean chanonce;
	private TextView mBatteryLevel;
	private TextView mChannelPrompt;
	private boolean mBatteryLevelRegistered;

    // ************************************************************** //
    // * ACTIVITY *************************************************** //
    // ************************************************************** //

    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	//chanToggle=false;
    	chanonce=false;
    	mBatteryLevelRegistered=false;
        Log.d(TAG, ">>> onCreate");
        super.onCreate(savedInstanceState);
        new CheckTask().execute();
        Log.d(TAG, "<<< onCreate");
        
        
        
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, ">>> onDestroy");
        super.onDestroy();
        Log.d(TAG, "<<< onDestroy");
    }

    private void onCreateImpl() {
    	setContentView(R.layout.channels);
    	Button bQuit=(Button)this.findViewById(R.id.quitBut);
    	bQuit.setOnClickListener(new View.OnClickListener() {
			public void onClick(View myView) {
				exitConfirm();
			}
    	});
    	Button bConfig=(Button)this.findViewById(R.id.configBut);
    	bConfig.setOnClickListener(new View.OnClickListener() {
			public void onClick(View myView) {
				openConfigMenu();
			}
    	});

    	
    	mBatteryLevel = (TextView) this.findViewById(R.id.batteryLevel);
        batteryLevel();
    	
        mChannelPrompt = (TextView) this.findViewById(R.id.channelName);
            	
        mChanList = (LinearLayout) findViewById(R.id.chanlist);
        mControlButs = (LinearLayout) findViewById(R.id.controlButs);

    	
        mVideoView = (VideoView) findViewById(R.id.videoFrame);
        
        mVideoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp) {
                Log.d(TAG, "onPrepared(" + mp + ")");
            }
        });
        mVideoView.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                Log.d(TAG, "onError(" + mp + "," + what + "," + extra + ")");
                //finish(); // return to channel list
                return true;
            }
        });
        mVideoView.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
                Log.d(TAG, "onCompletion(" + mp + ")");
                Toast.makeText(getBaseContext(), "video completion", Toast.LENGTH_SHORT).show();
                mp.reset();
                //finish(); // return to channel list
            }
        });
        mVideoView.setOnTouchListener(new View.OnTouchListener() {
			public boolean onTouch(View myView, MotionEvent event) {
				if (event.getAction() == MotionEvent.ACTION_UP) {
					if (mChanList.isShown()) {
						mChanList.setVisibility(View.GONE);
						mChannelPrompt.setVisibility(View.GONE);
						mControlButs.setVisibility(View.GONE);
					}
					else {
						mChanList.setVisibility(View.VISIBLE);
						mChannelPrompt.setVisibility(View.VISIBLE);
						mControlButs.setVisibility(View.VISIBLE);
					}
				}
					
				return true;
            }
        });
        mSpinner = (Spinner) findViewById(R.id.spinner);
        mSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view,
                    int position, long id) {
                loadChannels((String) parent.getSelectedItem());
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                loadChannels(null);
            }
        });

        mListView = (ListView) findViewById(R.id.listView);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position,
                    long id) {
                watchChannel((int) id);
            }
        });

        // start
        loadChannelLists();
        setChannelList(null);
        if (mSpinner.getAdapter().getCount() == 0) {
            PreferenceUtils.openSettings(this);
        }
    }

    private void openConfigMenu() {
		PreferenceUtils.openSettings(this);
    }
    private void batteryLevel() {
    	if (mBatteryLevelRegistered)
    		return;
        BroadcastReceiver batteryLevelReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                //context.unregisterReceiver(this);
                int rawlevel = intent.getIntExtra("level", -1);
                int scale = intent.getIntExtra("scale", -1);
                int level = -1;
                if (rawlevel >= 0 && scale > 0) {
                    level = (rawlevel * 100) / scale;
                }
                mBatteryLevel.setText("Battery Level Remaining: " + level + "%");
            }
        };
        IntentFilter batteryLevelFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        registerReceiver(batteryLevelReceiver, batteryLevelFilter);
        mBatteryLevelRegistered=true;
    }
    
    public void exitConfirm(){
        new AlertDialog.Builder(this)
        .setTitle(R.string.quit)
        .setMessage(R.string.really_quit)
        .setPositiveButton(R.string.yes, new OnClickListener() {
            public void onClick(DialogInterface arg0, int arg1) {
                //do stuff onclick of YES
            	tvStop();
                finish();
            }
        })
        .setNegativeButton(R.string.no, new OnClickListener() {
            public void onClick(DialogInterface arg0, int arg1) {
                //do stuff onclick of CANCEL
                Toast.makeText(getBaseContext(), "You touched CANCEL", Toast.LENGTH_SHORT).show();
            }
        }).show();
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            PreferenceUtils.openSettings(this);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    // ************************************************************** //
    // * CHANNEL UTILS ********************************************** //
    // ************************************************************** //

    private void loadChannelLists() {
        // reset channel lists spinner
        mSpinner.setAdapter(null);
        mListView.setAdapter(null);

        // load channel lists
        String[] configFiles = Utils.getConfigsDir(this).list();
        mSpinner.setAdapter(Utils.createSimpleArrayAdapter(this, configFiles));
    }

    private void loadChannels(String channelListName) {
        // reset channel list view
        mListView.setAdapter(null);

        // check channel list
        File file = Utils.getConfigsFile(this, channelListName);
        if (file == null || !file.canRead() || !file.isFile()) {
            return;
        }

        // load channels
        mChannelConfigs = null;
        mChannels = null;
        try {
            List<String> channelConfigList = new ArrayList<String>();
            List<String> channelList = new ArrayList<String>();
            BufferedReader reader = new BufferedReader(new FileReader(file));
            String line;
            while ((line = reader.readLine()) != null) {
                channelConfigList.add(line);
                channelList.add(line.split(":")[0]);
            }
            reader.close();
            mChannelConfigs = channelConfigList.toArray(new String[channelConfigList
                    .size()]);
            mChannels = channelList.toArray(new String[channelList.size()]);
        } catch (IOException e) {
            ErrorUtils.error(this, getText(R.string.error_invalid_channel_configuration), e);
        }

        // update channel list view
        mListView.setAdapter(new ArrayAdapter<String>(this, R.layout.list_item,
                mChannels));
    }

    /**
     * @param channelList list name, insert null to read config from settings
     */
    private void setChannelList(String channelList) {
        if (channelList == null) {
            channelList = PreferenceUtils.get(this).getString(PreferenceUtils.KEY_CHANNELCONFIGS, null);
        }

        if (StringUtils.isNullOrEmpty(channelList) || Utils.getConfigsFile(this,
                channelList) == null) {
            String[] files = Utils.getConfigsDir(this).list();
            if (files.length == 1) {
                channelList = files[0];
            } else {
                channelList = null;
            }
        }

        if (channelList == null) {
            mSpinner.setSelection(AdapterView.INVALID_POSITION);
        } else {
            for (int i = 0; i < mSpinner.getAdapter().getCount(); i++) {
                if (channelList.equals(mSpinner.getAdapter().getItem(i))) {
                    mSpinner.setSelection(i);
                    break;
                }
            }
        }
    }

    private void watchChannel(int channelId) {
        Log.d(TAG, "watchChannel(" + channelId + "): " + mChannels[channelId]);
        /*
        String channelconfig = mChannelConfigs[channelId];
        Intent intent = new Intent(this, StreamActivity.class);
        intent.putExtra(StreamActivity.EXTRA_CHANNELCONFIG, channelconfig);
        startActivity(intent);
        */
        mChannelConfig=mChannelConfigs[channelId];
        if (!chanonce) {

    		chanonce=true;
    		
    	}
        else
        	tvStop();
        tvPlay();
        /*
        if (!chanToggle) {
            tvPlay();
            chanToggle=true;
        }
        else {
        	tvStop();
        	chanToggle=false;
        }
        */
    }

    // ************************************************************** //
    // * INITIALIZATION TASK **************************************** //
    // ************************************************************** //

    private class CheckTask extends AsyncTask<Void, Integer, Integer> {

        static final int OK = 0;
        static final int CHECK_DEVICE = R.string.no_device;

        static final String LEGACY_DEV_DVB_ADAPTER = "/dev/dvb0";
        final File LEGACY_FILE_DVB_CA = new File(LEGACY_DEV_DVB_ADAPTER + ".ca0");
        final File LEGACY_FILE_DVB_FRONTEND = new File(LEGACY_DEV_DVB_ADAPTER + ".frontend0");
        final File LEGACY_FILE_DVB_DEMUX = new File(LEGACY_DEV_DVB_ADAPTER + ".demux0");
        final File LEGACY_FILE_DVB_DVR = new File(LEGACY_DEV_DVB_ADAPTER + ".dvr0");

        static final String DEV_DVB_ADAPTER = "/dev/dvb/adapter0";
        final File FILE_DVB_CA = new File(DEV_DVB_ADAPTER + "/ca0");
        final File FILE_DVB_FRONTEND = new File(DEV_DVB_ADAPTER + "/frontend0");
        final File FILE_DVB_DEMUX = new File(DEV_DVB_ADAPTER + "/demux0");
        final File FILE_DVB_DVR = new File(DEV_DVB_ADAPTER + "/dvr0");

        private boolean verifyDeviceNode(File deviceNode, File legacyDeviceNode, boolean checkExists) {
            if (checkExists) {
                if (deviceNode.exists()) {
                    if (!deviceNode.canRead() || !deviceNode.canWrite()) {
                        fixPermission(deviceNode);
                    }
                } else if (legacyDeviceNode.exists()) {
                    if (!legacyDeviceNode.canRead() || !legacyDeviceNode.canWrite()) {
                        fixPermission(legacyDeviceNode);
                    }
                    createSymlink(legacyDeviceNode, deviceNode);
                } else {
                    return false;
                }
                return deviceNode.exists() && deviceNode.canRead() && deviceNode.canWrite();
            } else {
                if (deviceNode.exists() || legacyDeviceNode.exists()) {
                    return verifyDeviceNode(deviceNode, legacyDeviceNode, true);
                } else {
                    return true;
                }
            }
        }

        private void fixPermission(File file) {
            try {
                Process p = ProcessUtils.runAsRoot("chmod", "666", file.getAbsolutePath());
                p.waitFor();
            } catch (Exception e) {
                Log.w(TAG, "Failed to fix permission: " + file.getAbsolutePath(), e);
            }
        }

        private void createSymlink(File target, File symlink) {
            try {
                Process p = ProcessUtils.runAsRoot(
                        "mkdir", "-p", symlink.getParentFile().getAbsolutePath(), "\n",
                        "ln", "-s", target.getAbsolutePath(), symlink.getAbsolutePath());
                p.waitFor();
            } catch (Exception e) {
                Log.w(TAG, "Failed to create symlink: " + target.getAbsolutePath() + " <- "
                        + symlink.getAbsolutePath());
            }
        }

        private boolean checkDevice() {
            return verifyDeviceNode(FILE_DVB_FRONTEND, LEGACY_FILE_DVB_FRONTEND, true)
                    && verifyDeviceNode(FILE_DVB_DEMUX, LEGACY_FILE_DVB_DEMUX, true)
                    && verifyDeviceNode(FILE_DVB_DVR, LEGACY_FILE_DVB_DVR, true)
                    && verifyDeviceNode(FILE_DVB_CA, LEGACY_FILE_DVB_CA, false);
        }

        @Override
        protected void onPreExecute() {
            setContentView(R.layout.loading);
        }

        @Override
        protected Integer doInBackground(Void... params) {
            // check for device access
            publishProgress(CHECK_DEVICE);
            // kill old instance if still running
            try {
                ProcessUtils.killBinary(getApplicationContext(), DVBLAST);
                ProcessUtils.killBinary(getApplicationContext(), DVBLASTCTL);
            } catch (IOException e) {
                Log.w(TAG, "kill previous instances", e);
            }
            if (!checkDevice())
                return CHECK_DEVICE;
            // everything fine
            return OK;
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            int value = values[0];
            TextView loading = (TextView) findViewById(R.id.loading);
            // update text
            switch (value) {
                case CHECK_DEVICE:
                    loading.setText(R.string.check_device);
                    break;
            }
        }

        @Override
        protected void onPostExecute(Integer result) {
            if (result == OK) {
                onCreateImpl();
            } else {
                // show alert and abort application
                ErrorUtils.error(ChannelsActivity.this, getText(result), new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        finish();
                    }
                });
            }
        }
    }
    
    
    // stream related
    private void tvPlay() {
        Log.d(TAG, "onStart");
        
        String name = startStream(mChannelConfig);
        Log.d(TAG, "startstream ok");
        if (name == null) {
            Log.d(TAG, "name is null");
            return;
        }
        mChannelName = name;
        mChannelPrompt.setText(name);
        startPlayback();
        Log.d(TAG, "startplayback ok");
        //updateTitle();
    }

    private void tvStop() {
        Log.d(TAG, "onStop");
        stopPlayback();
        stopStream();
    }
    
    /**
     * @param channelconfig
     * @return channel name
     */    
    private String startStream(String channelconfig) {
        try {
            Log.d(TAG, ">>> startStream(" + channelconfig + ")");
            try {
                removeSocketFile();
                // config file
                File configFile = new File(getCacheDir(), DVBLAST_CONFIG_FILENAME);
                PrintWriter writer = new PrintWriter(configFile);
                // sNAME/iFREQ/iServiceID
                String[] params = channelconfig.split(":");
                // check config length
                if (params.length != 3) {
                    throw new IOException("invalid DVB params count[" + params.length + "]");
                }
                // parse config
                String name = params[0];
                int freq = tryParseInt(params[1], "frequency");
                int sid = tryParseInt(params[2], "service ID");
                // print config
                writer.println(String.format(DVBLAST_CONFIG_CONTENT, sid));
                writer.close();
                // run dvblast
                Log.d(TAG, "dvblast(" + configFile + "," + freq + ")");
                mUdpSocket = new DatagramSocket(UDP_PORT, Inet4Address.getByName(UDP_IP));
                mUdpSocket.setSoTimeout(1000);
                mHttpSocket = new ServerSocket(HTTP_PORT, 10, Inet4Address.getByName(HTTP_IP));
                mStreamTask = new AsyncStreamTask();
                mStreamTask.execute();
                mDvblastTask = new AsyncDvblastTask(configFile, freq);
                mDvblastTask.execute();
                mDvblastCtlTask = new AsyncDvblastCtlTask();
                mDvblastCtlTask.execute();
                return name;
            } catch (IOException e) {
                Log.e(TAG, "starting stream failed", e);
                ErrorUtils.error(this, "failed to start streaming", e);
                return null;
            }
        } finally {
            Log.d(TAG, "<<< startStream");
        }
    }

    private void stopStream() {
        Log.d(TAG, ">>> stopStream");
        if (mUdpSocket != null) {
            mUdpSocket.close();
        }
        if (mHttpSocket != null) {
            try {
                mHttpSocket.close();
            } catch (IOException e) {
                // nop
            }
        }
        ProcessUtils.finishTask(mDvblastCtlTask, false);
        ProcessUtils.finishTask(mStreamTask, true);
        ProcessUtils.finishTask(mDvblastTask, true);
        Log.d(TAG, "<<< stopStream");
    }

    private void startPlayback() {
    	/*
        Intent intent = new Intent(Intent.ACTION_VIEW).setDataAndType(Uri.parse(SERVICE_URL), "video/*");

        startActivity(intent);
        */

    	Log.d(TAG, "playback of "+SERVICE_URL);
    	/*
    	if (!chanonce) {

    		chanonce=true;
            mVideoView.start();

    	}
    	else
*/
		Uri video = Uri.parse(SERVICE_URL);             
		//Uri video = Uri.parse("/sdcard/Video/Demo video.avi");             
        mVideoView.setVideoURI(video);
        mVideoView.start();

    }

    private void stopPlayback() {
        if (mVideoView.isPlaying()) {
            mVideoView.pause();
        }
    }

    class AsyncStreamTask extends ParallelTask {

        final String TAG = ChannelsActivity.TAG + "." + AsyncStreamTask.class.getSimpleName();

        @Override
        protected void doInBackground() {
            Log.d(TAG, ">>>");
            byte[] data = new byte[4096];
            DatagramPacket dataPacket = new DatagramPacket(data, data.length);
            while (!isCancelled()) {
                Socket client;
                try {
                    client = mHttpSocket.accept();
                } catch (IOException e) {
                    continue;
                }
                // TODO parse HTTP request
                try {
                    OutputStream out = client.getOutputStream();
                    out.write(HTTP_HEADER.getBytes());
                    out.flush();
                    while (!isCancelled()) {
                        try {
                            mUdpSocket.receive(dataPacket);
                            out.write(dataPacket.getData(), dataPacket.getOffset(),
                                    dataPacket.getLength());
                        } catch (InterruptedIOException e) {
                            Log.w(TAG, "udp timeout");
                            continue;
                        } catch (SocketException e) {
                            break;
                        }
                    }
                } catch (IOException e) {
                    Log.e(TAG, "STREAM", e);
                }
            }
            Log.d(TAG, "<<<");
        }
    }

    class AsyncDvblastCtlTask extends ParallelTask {

        final String TAG = ChannelsActivity.TAG + "." + AsyncDvblastCtlTask.class.getSimpleName();

        @Override
        protected void doInBackground() {
            Log.d(TAG, ">>>");
            try {
                Thread.sleep(DVBLAST_CHECKDELAY);
            } catch (InterruptedException e) {
                // nop
            }
            final FrontendStatus status = new FrontendStatus();
            mFrontendStatus = status;
            while (!isCancelled()) {
                try {
                    Process dvblastctl = ProcessUtils.runBinary(ChannelsActivity.this, DVBLASTCTL,
                            ENVP_TMPDIR,
                            "-r", DVBLAST_SOCKET, "-x", "xml", "fe_status");
                    int exitCode = dvblastctl.waitFor();
                    if (exitCode != 0) {
                        Log.w(TAG, "exited with " + exitCode);
                        continue;
                    }
                    Document dom = getDomElement(dvblastctl.getInputStream());
                    NodeList statusList = dom.getElementsByTagName("STATUS");
                    for (int i = 0; i < statusList.getLength(); i++) {
                        Node node = statusList.item(i);
                        String statusName = node.getAttributes().getNamedItem("status")
                                .getNodeValue();
                        if ("HAS_SIGNAL".equals(statusName)) {
                            status.status |= FrontendStatus.HAS_SIGNAL;
                        } else if ("HAS_CARRIER".equals(statusName)) {
                            status.status |= FrontendStatus.HAS_CARRIER;
                        } else if ("HAS_VITERBI".equals(statusName)) {
                            status.status |= FrontendStatus.HAS_VITERBI;
                        } else if ("HAS_SYNC".equals(statusName)) {
                            status.status |= FrontendStatus.HAS_SYNC;
                        } else if ("HAS_LOCK".equals(statusName)) {
                            status.status |= FrontendStatus.HAS_LOCK;
                        } else if ("REINIT".equals(statusName)) {
                            status.status |= FrontendStatus.REINIT;
                        }
                    }
                    NodeList valueList = dom.getElementsByTagName("VALUE");
                    for (int i = 0; i < valueList.getLength(); i++) {
                        Node node = valueList.item(i);
                        Node valueNode = node.getAttributes().item(0);
                        String valueName = valueNode.getNodeName();
                        String value = valueNode.getNodeValue();
                        if ("bit_error_rate".equalsIgnoreCase(valueName)) {
                            status.ber = Long.parseLong(value);
                        } else if ("signal_strength".equalsIgnoreCase(valueName)) {
                            status.signal = Integer.parseInt(value);
                        } else if ("snr".equalsIgnoreCase(valueName)) {
                            status.snr = Integer.parseInt(value);
                        }
                    }
                    publishProgress();
                } catch (Throwable t) {
                    Log.w(TAG, "dvblastctl", t);
                }
                // zZzZZZ..
                try {
                    Thread.sleep(DVBLAST_CHECKDELAY);
                } catch (InterruptedException e) {
                    continue;
                }
            }
            mFrontendStatus = null;
            Log.d(TAG, "<<<");
        }

        private void publishProgress() {
            Message msg = Message.obtain(mHandler, new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "fe_status: " + mFrontendStatus);
                    updateTitle();
                }
            });
            mHandler.sendMessage(msg);
        }

        private Document getDomElement(InputStream xmlStream) {
            Document doc = null;
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            try {
                DocumentBuilder db = dbf.newDocumentBuilder();
                InputSource is = new InputSource();
                is.setByteStream(xmlStream);
                doc = db.parse(is);
            } catch (ParserConfigurationException e) {
                Log.e(TAG, e.getMessage());
                return null;
            } catch (SAXException e) {
                Log.e("Error: ", e.getMessage());
                return null;
            } catch (IOException e) {
                Log.e("Error: ", e.getMessage());
                return null;
            }
            return doc;
        }

    }

    class AsyncDvblastTask extends ParallelTask {
        final String TAG = ChannelsActivity.TAG + "." + AsyncDvblastTask.class.getSimpleName();

        private File mErrorLog;
        private PrintWriter mErrorLogger;
        private File mConfigFile;
        private int mFreq;

        public AsyncDvblastTask(File configFile, int freq) {
            mConfigFile = configFile;
            mFreq = freq;
            mErrorLog = new File(getCacheDir(), DVBLAST_LOG);
            try {
                mErrorLogger = new PrintWriter(mErrorLog);
            } catch (FileNotFoundException e) {
                Log.wtf(TAG, "error logger", e);
            }
        }

        @Override
        protected void doInBackground() {
            Log.d(TAG, ">>>");
            try {
                Process dvblast = ProcessUtils.runBinary(ChannelsActivity.this, DVBLAST,
                        "-U", "-a0", "-O5000", "-r", DVBLAST_SOCKET,
                        "-xxml", "-c", mConfigFile.getAbsolutePath(),
                        "-f" + mFreq, "-q");
                Integer exitCode = null;
                while (!isCancelled() &&
                        (exitCode = ProcessUtils.checkExitCode(dvblast)) == null) {
                    try {
                        int read = 0;
                        String str;
                        // handle dvblast info
                        str = StringUtils.readAll(dvblast.getInputStream());
                        if (str.length() > 0) {
                            read += str.length();
                            // TODO parse DOM and handle
                        }
                        // read error log
                        str = StringUtils.readAll(dvblast.getErrorStream());
                        if (str.length() > 0) {
                            read += str.length();
                            mErrorLogger.write(str);
                        }
                        // zzZZzzz
                        if (read == 0) {
                            Thread.sleep(250);
                        }
                    } catch (Throwable t) {
                        break;
                    }
                }
                if (exitCode == null) {
                    Log.d(TAG, "dvblast destroying");
                    ProcessUtils.terminate(dvblast);
                    Log.d(TAG, "dvblast destroyed");
                } else if (exitCode != 0) {
                    // FIXME localization
                    Log.e(TAG, "dvblast failed (" + exitCode + ")");
                    Log.d(TAG, ProcessUtils.readStdOut(dvblast));
                    Log.d(TAG, ProcessUtils.readErrOut(dvblast));
                }
                removeSocketFile();
            } catch (Throwable t) {
                Log.e(TAG, "dvblast", t);
            }
            Log.d(TAG, "<<<");
        }
    }

    private static int tryParseInt(String str, String paramName)
            throws IOException {
        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException e) {
            throw new IOException(
                    "error while parsing " + paramName + " (" + str + ")");
        }
    }

    private void updateTitle() {
        String str = mChannelName;
        if (mFrontendStatus != null) {
            str += String.format("  [Signal: %2d%%, Error: %d]",
                    (mFrontendStatus.signal * 100 / FrontendStatus.SIGNAL_MAXVALUE),
                    (mFrontendStatus.ber));
        }
        setTitle(str);
    }

    private void removeSocketFile() {
        File f = new File(getCacheDir(), DVBLAST_SOCKET);
        if (f.exists() && !f.delete()) {
            Log.w(TAG, "unable to delete " + DVBLAST_SOCKET);
        }
    }

}
