<?php

include("conf.inc")
require('rutorrent-stats.inc');
$stats = rutorrent_stats($seedboxes);
print json_encode($stats);
exit;
