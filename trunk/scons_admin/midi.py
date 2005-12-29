# Reinhard Katzmann, 2005
# GNU license (see COPYING)

#General hints:
# TSE3 check is done in the method CheckTSE3
# Use builtin scons features for additional libraries
# TSE3 is now always used in favor of KMid.
# TSE3 checking cannot be disabled at this time
#
# IMPORTANT
# TSE3 has a depandency on another library called aRts related to KDE
# Avoiding such dependencies for other libraries makes life much easier
# Also don't use libraries such as TSE3 without pkconfig / kdeconfig support!

import os, re

# Might be useful in future (colors are defined there as well)
# import general

colors= {
'BOLD'  :"\033[1m",
'GREY'  :"\033[90m",
'RED'   :"\033[91m",
'GREEN' :"\033[92m",
'BLUE'  :"\033[94m",
'YELLOW':"\033[1m", #"\033[93m" # unreadable on white backgrounds
'CYAN'  :"\033[96m",
'NORMAL':"\033[0m",
}

def exists(env):
        return true

def generate(env):
	""" Detect the one midi (tse3) library """

        midilib  = '/usr/lib'
	withtse3 = 'yes'
	withkmid = 'no'
	
	p=env.pprint
	if env['HELP']:
                p('BOLD','*** MIDI options ***')
                p('BOLD','--------------------')
                p('BOLD','* midilib    ','install path for the lib, ie: /usr/lib')
                p('BOLD','* withtse3   ','use tse3 library (default: yes)')
                p('BOLD','* withkmid   ','use kmid library (default: no)')
                p('BOLD','* scons configure midilib=/usr/local/lib\n')
		return
	
        from SCons.Options import Options
        optionfile = env['CACHEDIR'] + 'midi.cache.py'
        opts = Options(optionfile)

	TSE3_FLAGS = None
        # TODO : add your own options here
        opts.AddOptions(
                ( 'midilib',  'install path for the lib, ie: /usr/lib' ),
                ( 'withtse3', 'use tse3 library (default: yes)' ),
                ( 'withkmid', 'use kmid library (default: no)' ),
        )
        opts.Update(env)

        # this condition is used to check when to redetect the configuration (do not remove)
	if 'configure' in env['TARGS'] or not env.has_key('ALREADY_CONFIGURED'):
		# Register our custom test to scons
		conf = env.Configure( custom_tests = { 'CheckTSE3' : CheckTSE3 } )
		# Base automatic TSE3 test (no version check) without asking :-)
		haveTSE3 = conf.CheckLibWithHeader('libtse3','tse3/TSE3.h','C++')
		# If TSE3 was found check version, aRts and adapt the environment
		if haveTSE3:
			if conf.CheckTSE3():
				conf.env.Append( CPPFLAGS = '-DWITH_TSE3=1' )
		# Base automatic KMid test (NEW: only if TSE3 is not installed)
		else:
			haveKMID = conf.CheckLibWithHeader('libkmid','libkmid/libkmid.h','C++')
			if haveKMID:
				conf.env.Append( CPPFLAGS = '-DWITH_LIBKMID=1' )
			else:
				p('RED','Warning: No midi libary found.')

		# We don't want to show all the build messages red / blue
		p('NORMAL','')
		env = conf.Finish()

                # success
                env['ALREADY_CONFIGURED']=1

                # clear options set previously
                if env.has_key('TSE3_FLAGS'):
                        env.__delitem__('TSE3_FLAGS')
                
                # .. and set them
                #env['TSE_FLAGS'] = ['-ltse3']
                
                # save the options
                opts.Save(optionfile, env)

        # the configuration is over - at this point XXX_FLAGS is defined

        # load the variables detected into the environment
        #env.AppendUnique( CXXFLAGS = env['TSE3_FLAGS'] )
        

def CheckTSE3(context):
	p = context.env.pprint
	context.Message(colors['NORMAL'] + 'Checking for TSE3 version... ')
	# Read version
	ret = context.TryRun("""
#include <istream.h>
#include <tse3/TSE3.h>
int main() {
        cout << TSE3::TSE3_Version() << endl;
        return 0;
}
""",'.cpp')
	if not ret[0]:
		context.Result(colors['RED'] + "failed")
		return 0
	# Split version into major, mid and minor
	tseVer = str(ret[1]).split( "." )
	tseVStr = tseVer[0] + '.' + tseVer[1] + '.' + tseVer[2]
	context.Result(colors['GREY'] + tseVStr)
	if int(tseVer[0]) == 0 and int(tseVer[1]) < 2:
		p('RED','Warning: Found old version of tse3. Please consider an update.')
	if int(tseVer[0]) < 0 or int(tseVer[1]) < 1 or (int(tseVer[1]) < 2 and int(tseVer[2]) < 2):
		p('RED','Error: Version ' + tseVStr + ' of TSE3 is too old. Please update.')
		sys.exit(0);
	# Set Version environment for midimapper
	if int(tseVer[1]) > 1:
		context.env.Append( CPPFLAGS = '-DTSE3_MID_VERSION_NR=2' )
	else:
		context.env.Append( CPPFLAGS = '-DTSE3_MID_VERSION_NR=1' )
	# Test for aRts if KDE has aRts Library installed
	context.Message( colors['NORMAL'] + 'Checking for TSE3 with aRts... ')
	if int(tseVer[1]) > 1:
		ret = context.TryRun("""
#include <istream.h>
#include <tse3/plt/Factory.h>
#include <tse3/plt/Arts.h>
int main() {
        TSE3::MidiSchedulerFactory theFactory_;
        TSE3::MidiScheduler           *theScheduler_;
        TSE3::Plt::UnixMidiSchedulerFactory::setPreferredPlatform(TSE3::Plt::UnixMidiSchedulerFactory::UnixPlatform_Alsa);
        theScheduler_ = theFactory_.createScheduler();
	cout << "ok" << endl;
        return 0;
}
""",'.cpp')

	else:
		ret = context.TryRun("""
#include <istream.h>
#include <tse3/plt/Arts.h>
int main() {
        TSE3::Plt::ArtsMidiSchedulerFactory theARtsFactory_;
        TSE3::MidiScheduler           *theScheduler_;
        theScheduler_ = theARtsFactory_.createScheduler();
	cout << "ok" << endl;
        return 0;
}
""",'.cpp')
	# Set Arts environment for midimapper
	if ret[0]:
		context.Result( colors['BLUE'] + 'Yes')
		context.env.Append( CPPFLAGS = '-DTSE3_HAS_ARTS=1' )
	else:
		context.Result( colors['RED'] + 'No')
	return 1
