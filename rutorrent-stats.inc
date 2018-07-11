<?php

function rutorrent_stats($seedboxes) {
  $stats = array();
  foreach ($seedboxes as $name => $seedbox) {
    $stats[$name] = _get_seedbox_statistics($seedbox['url'], $seedbox['username'], $seedbox['password'], $seedbox['authtype'], $seedbox['rpc']);
  }
  return $stats;
}

function _get_seedbox_statistics($url, $username, $password, $authtype, $rpc) {
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, $url);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_VERBOSE, 1);
  curl_setopt($ch, CURLOPT_HTTPPROXYTUNNEL, TRUE);
  curl_setopt($ch, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
  curl_setopt($ch, CURLOPT_USERAGENT, "DriverPacks.net seedbox status checker/1.0 (Linux)");
  curl_setopt($ch, CURLOPT_TIMEOUT, 2);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
  curl_setopt($ch, CURLOPT_POST, 1);

  curl_setopt($ch, CURLOPT_HTTPAUTH, $authtype);
  curl_setopt($ch, CURLOPT_USERPWD, "$username:$password");

  $f = "_rutorrent_stats_${rpc}_generate_request";
  $request = $f();
  curl_setopt($ch, CURLOPT_HTTPHEADER, $request->headers);
  curl_setopt($ch, CURLOPT_POSTFIELDS, $request->post);
  $result = curl_exec($ch);

  // Detect errors.
  if (curl_errno($ch)) {
    return array(
      'online' => FALSE,
      'offline-reason' => curl_error($ch),
    );
  }

  // Ensure authentication worked and that the RPC URL is correct.
  $http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
  if ($http_code == 401) {
    return array(
      'online' => FALSE,
      'offline-reason' => 'Authentication failed.',
    );
  }
  else if ($http_code == 404) {
    return array(
      'online' => FALSE,
      'offline-reason' => 'Bad RPC URL: got a 404.',
    );
  }

  // Close connection.
  curl_close($ch);

  // Work with the data.
  $f = "_rutorrent_stats_${rpc}_decode_answer";
  $answer = $f($result);
  if ($answer == false) {
    return array(
      'online' => FALSE,
      'offline-reason' =>'parse error answer',
    );
  }
  $num_torrents = count(array_keys($answer));

  // Gather statistics.
  $f = "_rutorrent_stats_${rpc}_get_indices";
  $indices = $f();
  if ($indices == false) {
    return array(
      'online' => FALSE,
      'offline-reason' =>'parse error indices',
    );

  }
  $size = 0;
  $ul_speed = 0;
  $ul_size = 0;
  $ul_count = 0;
  $peers = 0;
  foreach ($answer as $torrent_details) {
    $torrent_size     = $torrent_details[$indices['size']    ];
    $torrent_ul_count = $torrent_details[$indices['ul_count']] / 1000;
    $torrent_ul_size  = $torrent_size * $torrent_ul_count;

    $size     += $torrent_size;
    $ul_speed += $torrent_details[$indices['ul_speed']];
    $ul_count += $torrent_ul_count;
    $ul_size  += $torrent_ul_size;
    $peers    += $torrent_details[$indices['peers']   ];
  }
  $stats = array(
    'online'           => ($num_torrents > 0),
    'torrents'         => $num_torrents,
    'size'             => round($size / (1000 * 1000 * 1000), 2), // GB
    'upload_speed'     => round($ul_speed / 1000), // KBps
    'upload_size'      => round($ul_size / (1000 * 1000 * 1000), 2), // GB
    'upload_count_avg' => round($ul_count / $num_torrents, 2),
    'peers'            => $peers
  );

  return $stats;
}

function _rutorrent_stats_httprpc_generate_request() {
  $args = array(
    'mode' => 'list',
    'cmd' => array(
      'd.get_hash=',
      'd.get_name=',
      'd.get_bytes_done=',
      'd.get_up_total=',
      'd.get_up_rate=',
      'd.get_ratio=',
      'd.get_peers_connected=',
    )
  );

  // Build POST request from RPC args.
  $post = array();
  foreach ($args as $key => $value) {
    if (is_array($value)) {
      foreach (array_values($value) as $v) {
        $post[] = urlencode($key) . "=" . urlencode($v);
      }
    }
    else {
      $post[] = urlencode($key) . "=" . urlencode($value);
    }
  }
  $post = implode("&", $post);

  $request = new StdClass();
  $request->post = $post;
  $request->headers = array(
    'Accept: application/json, text/javascript',
  );

  return $request;
}

function _rutorrent_stats_httprpc_decode_answer($answer) {
  $decoded = json_decode($answer);
  $data = (array) $decoded->t;
  return $data;
}

function _rutorrent_stats_httprpc_get_indices() {
  return array(
    'size'     => 8,
    'ul_speed' => 11,
    'ul_count' => 10,
    'peers'    => 15,
  );
}

function _rutorrent_stats_xmlrpc_generate_request() {
  $request = new StdClass();
  $request->post = <<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<methodCall>
  <methodName>d.multicall</methodName>
  <params>
    <param><value><string>main</string></value></param>
    <param><value><string>d.get_hash=</string></value></param>
    <param><value><string>d.get_name=</string></value></param>
    <param><value><string>d.get_bytes_done=</string></value></param>
    <param><value><string>d.get_up_rate=</string></value></param>
    <param><value><string>d.get_ratio=</string></value></param>
    <param><value><string>d.get_peers_connected=</string></value></param>
  </params>
</methodCall>
EOF;
  $request->headers = array(
    'Content-type: text/xml',
  );

  return $request;
}

function _rutorrent_stats_xmlrpc_decode_answer($answer) {
  include_once('xmlrpc.inc');

  // PHP's xmlrpc_decode() doesn't support 64-bit ints, nor do we need it
  // here, hence use the hack described at
  // http://www.php.net/manual/en/function.xmlrpc-decode.php#93080
  $answer = str_replace('i8>', 'i4>', $answer);
  // Now parse what we've got back
  $message = xmlrpc_message($answer);
  if (!xmlrpc_message_parse($message)) {
    return false;
  }
  $data = $message->params[0];

  return $data;
}

function _rutorrent_stats_xmlrpc_get_indices() {
  return array(
    'size'     => 2,
    'ul_speed' => 3,
    'ul_count' => 4,
    'peers'    => 5,
  );
}
