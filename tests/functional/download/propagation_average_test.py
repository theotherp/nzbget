import re
import time

nzbget_options = ['PropagationDelay=15', 'PropagationDelayMode=average']

def make_nzb(nzbget):
	# small.dat was posted an hour ago, the obfuscated file just now (like a late nfo or par2);
	# the average post time is half an hour ago, which is older than the propagation delay
	nzb_content = nzbget.load_nzb('small.nzb')
	nzb_content = re.sub(r'date="\d+"', 'date="%i"' % (time.time() - 3600), nzb_content)
	late_file = re.search(r'<file .*?</file>', nzbget.load_nzb('small-obfuscated.nzb'), re.S).group(0)
	late_file = re.sub(r'date="\d+"', 'date="%i"' % time.time(), late_file)
	return nzb_content.replace('</nzb>', late_file + '\n</nzb>')

def test_propagation_delay_average_downloads_late_file(nserv, nzbget):
	hist = nzbget.download_nzb('propagation_average.nzb', make_nzb(nzbget))
	assert hist['Status'] == 'SUCCESS/HEALTH'
	assert hist['FileCount'] == 2
	assert hist['MaxPostTime'] - hist['AvgPostTime'] >= 1500
