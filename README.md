# rutorrent-stats

rutorrent-stats is a small PHP script that makes it possible to check the vitals of one or more seedboxes running [`rtorrent`][rtorrent] with a [`rutorrent`][rutorrent] front-end (Web UI).

## html fronted

![fronted](https://i.imgur.com/HA4kc9e.png)


## Live sites
`rutorrent-stats` is used in a production environment at <http://driverpacks.net> — you can see the results at the [downloads][dps.net-downloads] and [statistics][dps.net-stats] pages. There, they are used in tandem with the [OpenTracker Drupal module][opentracker-module].


## Usage
You need to include `rutorrent-stats.inc`. Then use the function `rutorrent_stats()`, which is the only public function exposed by `rutorrent-stats`.
`rutorrent_stats()` accepts an array of arrays with seedbox authentication credentials. The keys of these nested arrays are the names of the seedboxes (and can contain *any* value). The nested arrays themselves then, contain the following key-value pairs:

* `'url'`: the URL where RPC requests should be sent.
* `'username'`
* `'password'`
* `'authtype'`: either `CURLAUTH_BASIC` or `CURLAUTH_DIGEST`
* `'rpc'`: either `'httprpc'` or `'xmlrpc'`

For example:

	$seedboxes = array(
	  'seedbox company name' => array(
	    'url'      => 'http://seedboxcompany.com/plugins/httprpc/action.php',
	    'username' => 'your username',
	    'password' => 'your password,
	    'authtype' => CURLAUTH_BASIC,
	    'rpc'      => 'httprpc',
	  ),
	);
	require('rutorrent-stats.inc');
	$stats = rutorrent_stats($seedboxes);
	print json_encode($stats);
	exit;

This example, but with two seedboxes, is also in the repository: `rutorrent-stats.php`.


## Requirements
* PHP 4 or 5
* PHP compiled with `curl` support


## State
While unit tests are missing, the scope of this project does not warrant the required effort at this time — especially because any update of `rutorrent` may break these scripts.


## How?
It does this by taking advantage of `rutorrent`'s RPC functionality. It supports both `rutorrent`'s `HTTPRPC` (which actually uses JSON-RPC) and `RPC` (which actually uses XML-RPC) plug-ins. If you have the choice, use `HTTPRPC`, because it requires less bandwidth (and thus also less parsing effort).

Due to the vagueness surrounding `rutorrent`, I have no idea if this is compatible with future or previous versions of `rutorrent`. This was developed against two seedboxes that both run version 3.2 of `rutorrent` (and thus also version 3.2 of either RPC plug-in).

The included XML-RPC library in PHP is copied verbatim from [Drupal 7.7][drupal-7.7].


## Future plans

* Nagios integration


## Changelog

* 0.1
    * Initial release. Support for multiple seedboxes and `rutorrent`'s `HTTPRPC` and `RPC` plug-ins. Compatible with `rutorrent` 3.2. Support for both Basic and Digest HTTP authentication (courtesy of `curl`).

[rtorrent]: http://libtorrent.rakshasa.no/
[rutorrent]: http://code.google.com/p/rutorrent/
[dps.net-downloads]: http://driverpacks.net/downloads
[dps.net-stats]: http://driverpacks.net/stats
[opentracker-module]: http://drupal.org/project/opentracker
[drupal-7.7]: http://drupal.org/drupal-7.7
