<?php
	require_once("dblib.php");

	const RESERVATION_LEAD_SECONDS = 60;
	const RESERVATION_GRACE_SECONDS = 60;

	function json_response($status, $payload)
	{
		http_response_code($status);
		header("Content-Type: application/json");
		print json_encode($payload);
		exit;
	}

	function request_body()
	{
		$raw = file_get_contents("php://input");
		if ($raw === false || trim($raw) === "")
		{
			return $_POST;
		}
		$decoded = json_decode($raw, true);
		if (!is_array($decoded))
		{
			json_response(400, array("ok" => false, "error" => "invalid_json"));
		}
		return $decoded;
	}

	function require_value($body, $name)
	{
		if (!isset($body[$name]) || $body[$name] === "")
		{
			json_response(422, array("ok" => false, "error" => "missing_" . $name));
		}
		return $body[$name];
	}

	function optional_value($body, $name, $default = null)
	{
		return isset($body[$name]) && $body[$name] !== "" ? $body[$name] : $default;
	}

	function parse_epoch($value)
	{
		if ($value === null || $value === "")
		{
			return null;
		}
		if (is_numeric($value))
		{
			$epoch = (float)$value;
			if ($epoch > 1000000000000)
			{
				$epoch = $epoch / 1000000;
			}
			else if ($epoch > 10000000000)
			{
				$epoch = $epoch / 1000;
			}
			return (int)$epoch;
		}
		$epoch = strtotime($value);
		if ($epoch === false)
		{
			json_response(422, array("ok" => false, "error" => "invalid_timestamp", "value" => $value));
		}
		return $epoch;
	}

	function bool_value($value)
	{
		return ($value === true || $value === "t" || $value === "true" || $value === "1" || $value === 1) ? "t" : "f";
	}

	function client_ip()
	{
		return isset($_SERVER["REMOTE_ADDR"]) ? $_SERVER["REMOTE_ADDR"] : "";
	}

	function bearer_token()
	{
		$headers = function_exists("getallheaders") ? getallheaders() : array();
		foreach ($headers as $name => $value)
		{
			if (strtolower($name) === "authorization" && preg_match('/^Bearer\s+(.+)$/i', $value, $matches))
			{
				return trim($matches[1]);
			}
			if (strtolower($name) === "x-api-token")
			{
				return trim($value);
			}
		}
		if (isset($_SERVER["HTTP_AUTHORIZATION"]) && preg_match('/^Bearer\s+(.+)$/i', $_SERVER["HTTP_AUTHORIZATION"], $matches))
		{
			return trim($matches[1]);
		}
		return "";
	}

	function hash_equals_safe($known, $user)
	{
		if (function_exists("hash_equals"))
		{
			return hash_equals($known, $user);
		}
		if (strlen($known) !== strlen($user))
		{
			return false;
		}
		$result = 0;
		for ($i = 0; $i < strlen($known); $i++)
		{
			$result |= ord($known[$i]) ^ ord($user[$i]);
		}
		return $result === 0;
	}

	function ip_matches($allowed, $ip)
	{
		$allowed = trim($allowed);
		if ($allowed === "")
		{
			return false;
		}
		if ($allowed === $ip)
		{
			return true;
		}
		if (strpos($allowed, "/") === false)
		{
			return false;
		}
		list($subnet, $bits) = explode("/", $allowed, 2);
		$ip_long = ip2long($ip);
		$subnet_long = ip2long($subnet);
		if ($ip_long === false || $subnet_long === false || !is_numeric($bits))
		{
			return false;
		}
		$bits = (int)$bits;
		if ($bits < 0 || $bits > 32)
		{
			return false;
		}
		$mask = $bits == 0 ? 0 : (-1 << (32 - $bits));
		return (($ip_long & $mask) === ($subnet_long & $mask));
	}

	function authenticate($db)
	{
		$token = bearer_token();
		$token_hash = $token !== "" ? hash("sha256", $token) : "";
		$ip = client_ip();

		$sth = $db->prepare("SELECT id, token_hash, allowed_ip FROM remotecontrolclients WHERE active='t'");
		$sth->execute();
		$clients = $sth->fetchAll(PDO::FETCH_ASSOC);
		foreach ($clients as $client)
		{
			$token_ok = $token_hash !== "" && $client["token_hash"] !== null &&
				hash_equals_safe($client["token_hash"], $token_hash);
			$ip_ok = $client["allowed_ip"] !== null && ip_matches($client["allowed_ip"], $ip);
			if ($token_ok || $ip_ok)
			{
				$upd = $db->prepare("UPDATE remotecontrolclients SET last_used=now() WHERE id=:id");
				$upd->execute(array(":id" => $client["id"]));
				return;
			}
		}

		json_response(403, array("ok" => false, "error" => "not_authorized"));
	}

	function resolve_interface($db, $body, $required_mode = null)
	{
		$identifier = optional_value($body, "streaminterface_id", optional_value($body, "streaminterface", optional_value($body, "interface")));
		if ($identifier === null)
		{
			json_response(422, array("ok" => false, "error" => "missing_streaminterface"));
		}

		$where = is_numeric($identifier) ? "si.id=:id" : "si.name=:name";
		$sql = "SELECT si.id AS streaminterface_id, si.name, sic.id AS config_id, sic.mode " .
			"FROM streaminterfaces si JOIN streaminterfaceconfigs sic ON sic.streaminterface_id=si.id " .
			"WHERE " . $where;
		if ($required_mode !== null)
		{
			$sql .= " AND sic.mode=:mode";
		}
		$sql .= " ORDER BY sic.id LIMIT 1";

		$sth = $db->prepare($sql);
		if (is_numeric($identifier))
		{
			$sth->bindValue(":id", (int)$identifier, PDO::PARAM_INT);
		}
		else
		{
			$sth->bindValue(":name", $identifier);
		}
		if ($required_mode !== null)
		{
			$sth->bindValue(":mode", $required_mode);
		}
		$sth->execute();
		$row = $sth->fetch(PDO::FETCH_ASSOC);
		if ($row === false)
		{
			json_response(404, array("ok" => false, "error" => "streaminterface_not_found"));
		}
		return $row;
	}

	function resolve_player_output($db, $body)
	{
		$body["streaminterface_id"] = optional_value($body, "player_streaminterface_id", optional_value($body, "streaminterface_id"));
		$body["streaminterface"] = optional_value($body, "player", optional_value($body, "streaminterface"));
		$iface = resolve_interface($db, $body, "player");
		$sth = $db->prepare("SELECT id AS sink_id, host, port FROM streaminterfacesinks WHERE streaminterfaceconfig_id=:config_id ORDER BY id LIMIT 1");
		$sth->execute(array(":config_id" => $iface["config_id"]));
		$sink = $sth->fetch(PDO::FETCH_ASSOC);
		if ($sink === false || (int)$sink["port"] === 0 || trim($sink["host"]) === "")
		{
			json_response(409, array("ok" => false, "error" => "player_has_no_output"));
		}
		return array_merge($iface, $sink);
	}

	function cleanup_reservations($db)
	{
		$db->exec("UPDATE outputreservations SET state='expired', updated_at=now() WHERE state='active' AND reserved_until < now()");
	}

	function player_process_active($name)
	{
		$pidfile = "/var/run/udp_player_" . $name . ".pid";
		if (!is_readable($pidfile))
		{
			return false;
		}
		$pid = (int)trim(file_get_contents($pidfile));
		if ($pid < 1)
		{
			return false;
		}
		if (function_exists("posix_kill"))
		{
			return posix_kill($pid, 0);
		}
		exec("kill -0 " . $pid . " 2>/dev/null", $out, $ret);
		return $ret === 0;
	}

	function output_conflict($db, $sink, $from_epoch, $until_epoch, $reservation_id = null)
	{
		$params = array(
			":host" => $sink["host"],
			":port" => $sink["port"],
			":from_epoch" => $from_epoch,
			":until_epoch" => $until_epoch
		);
		$sql = "SELECT id, owner, request_id FROM outputreservations " .
			"WHERE state='active' AND host=:host AND port=:port " .
			"AND reserved_from < to_timestamp(:until_epoch) " .
			"AND reserved_until > to_timestamp(:from_epoch)";
		if ($reservation_id !== null)
		{
			$sql .= " AND id<>:reservation_id";
			$params[":reservation_id"] = $reservation_id;
		}
		$sql .= " LIMIT 1";
		$sth = $db->prepare($sql);
		$sth->execute($params);
		$reservation = $sth->fetch(PDO::FETCH_ASSOC);
		if ($reservation !== false)
		{
			return array("type" => "reservation", "details" => $reservation);
		}

		$sth = $db->prepare(
			"SELECT si.name, ps.streaminterface_id FROM playerstats ps " .
			"JOIN streaminterfaceconfigs sic ON sic.id=ps.streaminterface_id " .
			"JOIN streaminterfaces si ON si.id=sic.streaminterface_id " .
			"JOIN streaminterfacesinks sis ON sis.streaminterfaceconfig_id=sic.id " .
			"WHERE ps.state='play' AND sis.host=:host AND sis.port=:port LIMIT 1"
		);
		$sth->execute(array(":host" => $sink["host"], ":port" => $sink["port"]));
		$player = $sth->fetch(PDO::FETCH_ASSOC);
		if ($player !== false && player_process_active($player["name"]))
		{
			return array("type" => "player", "details" => $player);
		}

		return null;
	}

	function with_output_lock($db, $callback)
	{
		$db->beginTransaction();
		try
		{
			$db->exec("LOCK TABLE outputreservations IN EXCLUSIVE MODE");
			cleanup_reservations($db);
			$result = $callback();
			$db->commit();
			return $result;
		}
		catch (Exception $e)
		{
			$db->rollBack();
			throw $e;
		}
	}

	function signal_player($name)
	{
		$pidfile = "/var/run/udp_player_" . $name . ".pid";
		if (!is_readable($pidfile))
		{
			return array("sent" => false, "warning" => "player_pid_not_found");
		}
		$pid = (int)trim(file_get_contents($pidfile));
		if ($pid < 1)
		{
			return array("sent" => false, "warning" => "invalid_player_pid");
		}
		if (function_exists("posix_kill"))
		{
			$sigusr1 = defined("SIGUSR1") ? SIGUSR1 : 10;
			return array("sent" => posix_kill($pid, $sigusr1));
		}
		exec("kill -USR1 " . $pid, $out, $ret);
		return array("sent" => $ret === 0);
	}

	function create_interface($db, $body)
	{
		$name = require_value($body, "name");
		$mode = require_value($body, "mode");
		if (!in_array($mode, array("recorder", "player", "delayer")))
		{
			json_response(422, array("ok" => false, "error" => "invalid_mode"));
		}

		$sth = $db->prepare("INSERT INTO streaminterfaces (interfacetype, name, usertext) VALUES (:type, :name, :usertext) RETURNING id");
		$sth->execute(array(
			":type" => optional_value($body, "interfacetype", "streamtransporter"),
			":name" => $name,
			":usertext" => optional_value($body, "usertext", "")
		));
		$streaminterface_id = $sth->fetchColumn();

		$sth = $db->prepare("INSERT INTO streaminterfaceconfigs (streaminterface_id, typeofconfig, autostart, msbuffer, resendmode, mode) " .
			"VALUES (:streaminterface_id, 'stored', :autostart, :msbuffer, :resendmode, :mode) RETURNING id");
		$sth->execute(array(
			":streaminterface_id" => $streaminterface_id,
			":autostart" => bool_value(optional_value($body, "autostart", false)),
			":msbuffer" => optional_value($body, "msbuffer", 0),
			":resendmode" => optional_value($body, "resendmode", "normal"),
			":mode" => $mode
		));

		json_response(201, array("ok" => true, "streaminterface_id" => (int)$streaminterface_id, "config_id" => (int)$sth->fetchColumn()));
	}

	function add_input($db, $body)
	{
		$iface = resolve_interface($db, $body);
		$sth = $db->prepare("INSERT INTO streaminterfacesources " .
			"(streaminterfaceconfig_id, name, priority, sourcetype, host, port, talkbackhost, talkbackport, recording_id, addrtp) " .
			"VALUES (:config_id, :name, :priority, :sourcetype, :host, :port, :talkbackhost, :talkbackport, :recording_id, :addrtp) RETURNING id");
		$sth->execute(array(
			":config_id" => $iface["config_id"],
			":name" => optional_value($body, "name", "source"),
			":priority" => optional_value($body, "priority", 0),
			":sourcetype" => optional_value($body, "type", optional_value($body, "sourcetype", "udp")),
			":host" => require_value($body, "host"),
			":port" => require_value($body, "port"),
			":talkbackhost" => optional_value($body, "talkbackhost", ""),
			":talkbackport" => optional_value($body, "talkbackport", null),
			":recording_id" => optional_value($body, "recording_id", null),
			":addrtp" => bool_value(optional_value($body, "addrtp", false))
		));
		json_response(201, array("ok" => true, "source_id" => (int)$sth->fetchColumn()));
	}

	function add_output($db, $body)
	{
		$iface = resolve_interface($db, $body);
		$sth = $db->prepare("INSERT INTO streaminterfacesinks " .
			"(streaminterfaceconfig_id, name, sinktype, host, port, talkbackhost, talkbackport, ttl, maxbps, rawudp, recording_id) " .
			"VALUES (:config_id, :name, :sinktype, :host, :port, :talkbackhost, :talkbackport, :ttl, :maxbps, :rawudp, :recording_id) RETURNING id");
		$sth->execute(array(
			":config_id" => $iface["config_id"],
			":name" => optional_value($body, "name", "sink"),
			":sinktype" => optional_value($body, "type", optional_value($body, "sinktype", "udp")),
			":host" => require_value($body, "host"),
			":port" => require_value($body, "port"),
			":talkbackhost" => optional_value($body, "talkbackhost", ""),
			":talkbackport" => optional_value($body, "talkbackport", null),
			":ttl" => optional_value($body, "ttl", 7),
			":maxbps" => optional_value($body, "maxbps", null),
			":rawudp" => bool_value(optional_value($body, "rawudp", false)),
			":recording_id" => optional_value($body, "recording_id", null)
		));
		json_response(201, array("ok" => true, "sink_id" => (int)$sth->fetchColumn()));
	}

	function reserve_output($db, $body)
	{
		$sink = resolve_player_output($db, $body);
		$now = time();
		$start = parse_epoch(optional_value($body, "startplayat", $now));
		if ($start < $now)
		{
			$start = $now;
		}
		if ($start - $now > RESERVATION_LEAD_SECONDS)
		{
			json_response(409, array("ok" => false, "error" => "reservation_too_early", "max_lead_seconds" => RESERVATION_LEAD_SECONDS));
		}
		$until = $start + RESERVATION_GRACE_SECONDS;
		$owner = optional_value($body, "owner", "remote-api");
		$request_id = optional_value($body, "request_id", null);

		$result = with_output_lock($db, function() use ($db, $sink, $now, $until, $owner, $request_id) {
			if ($request_id !== null)
			{
				$sth = $db->prepare("SELECT id FROM outputreservations WHERE request_id=:request_id LIMIT 1");
				$sth->execute(array(":request_id" => $request_id));
				$existing_id = $sth->fetchColumn();
				if ($existing_id !== false)
				{
					return (int)$existing_id;
				}
			}
			$conflict = output_conflict($db, $sink, $now, $until);
			if ($conflict !== null)
			{
				json_response(409, array("ok" => false, "error" => "output_in_use", "conflict" => $conflict));
			}
			$sth = $db->prepare("INSERT INTO outputreservations " .
				"(streaminterfacesink_id, streaminterface_id, host, port, reserved_from, reserved_until, owner, request_id) " .
				"VALUES (:sink_id, :streaminterface_id, :host, :port, to_timestamp(:from_epoch), to_timestamp(:until_epoch), :owner, :request_id) RETURNING id");
			$sth->execute(array(
				":sink_id" => $sink["sink_id"],
				":streaminterface_id" => $sink["streaminterface_id"],
				":host" => $sink["host"],
				":port" => $sink["port"],
				":from_epoch" => $now,
				":until_epoch" => $until,
				":owner" => $owner,
				":request_id" => $request_id
			));
			return (int)$sth->fetchColumn();
		});

		json_response(201, array("ok" => true, "reservation_id" => $result, "reserved_until" => gmdate("c", $until)));
	}

	function release_output($db, $body)
	{
		$id = require_value($body, "reservation_id");
		$sth = $db->prepare("UPDATE outputreservations SET state='released', updated_at=now() WHERE id=:id AND state='active'");
		$sth->execute(array(":id" => $id));
		json_response(200, array("ok" => true, "released" => $sth->rowCount()));
	}

	function playback_command($db, $body)
	{
		$sink = resolve_player_output($db, $body);
		$action = optional_value($body, "player_action", optional_value($body, "command", null));
		if ($action === null)
		{
			$action = optional_value($body, "jumptopos", null) !== null ? "jump" : "play";
		}
		if (!in_array($action, array("jump", "play", "pause", "playto")))
		{
			json_response(422, array("ok" => false, "error" => "invalid_player_action"));
		}

		$now = time();
		$startplayat = parse_epoch(optional_value($body, "startplayat", null));
		$check_from = $now;
		$check_until = $startplayat !== null && $startplayat > $now ? $startplayat + RESERVATION_GRACE_SECONDS : $now + RESERVATION_GRACE_SECONDS;
		if ($startplayat !== null && $startplayat - $now > RESERVATION_LEAD_SECONDS)
		{
			json_response(409, array("ok" => false, "error" => "playback_too_early_without_reservation", "max_lead_seconds" => RESERVATION_LEAD_SECONDS));
		}

		$reservation_id = optional_value($body, "reservation_id", null);
		with_output_lock($db, function() use ($db, $sink, $check_from, $check_until, $reservation_id) {
			if ($reservation_id !== null)
			{
				$sth = $db->prepare("SELECT id FROM outputreservations WHERE id=:id AND state='active' AND host=:host AND port=:port AND reserved_until >= now()");
				$sth->execute(array(":id" => $reservation_id, ":host" => $sink["host"], ":port" => $sink["port"]));
				if ($sth->fetch(PDO::FETCH_ASSOC) === false)
				{
					json_response(409, array("ok" => false, "error" => "reservation_not_available"));
				}
			}
			$conflict = output_conflict($db, $sink, $check_from, $check_until, $reservation_id);
			if ($conflict !== null)
			{
				json_response(409, array("ok" => false, "error" => "output_in_use", "conflict" => $conflict));
			}
			if ($reservation_id !== null)
			{
				$sth = $db->prepare("UPDATE outputreservations SET state='used', updated_at=now() WHERE id=:id");
				$sth->execute(array(":id" => $reservation_id));
			}
		});

		$recording_id = optional_value($body, "recording_id", null);
		if ($recording_id !== null)
		{
			$upd = $db->prepare("UPDATE streaminterfacesources SET recording_id=:recording_id WHERE streaminterfaceconfig_id=:config_id");
			$upd->execute(array(":recording_id" => $recording_id, ":config_id" => $sink["config_id"]));
			$upd = $db->prepare("UPDATE streaminterfacesinks SET recording_id=:recording_id WHERE streaminterfaceconfig_id=:config_id");
			$upd->execute(array(":recording_id" => $recording_id, ":config_id" => $sink["config_id"]));
		}

		$sth = $db->prepare("INSERT INTO playercoms (streaminterface_id, pathnumber, action, jumptopos, startplayat, playto) " .
			"VALUES (:streaminterface_id, 0, :action, to_timestamp(CAST(:jumptopos AS double precision)), " .
			"to_timestamp(CAST(:startplayat AS double precision)), to_timestamp(CAST(:playto AS double precision)))");
		$sth->execute(array(
			":streaminterface_id" => $sink["config_id"],
			":action" => $action,
			":jumptopos" => parse_epoch(optional_value($body, "jumptopos", null)),
			":startplayat" => $startplayat,
			":playto" => parse_epoch(optional_value($body, "playto", null))
		));

		$signal = signal_player($sink["name"]);
		json_response(202, array("ok" => true, "player" => $sink["name"], "output" => $sink["host"] . ":" . $sink["port"], "signal" => $signal));
	}

	function process_command($body)
	{
		$process = require_value($body, "process");
		$command = require_value($body, "process_command");
		$interface = require_value($body, "interface");
		if (!in_array($process, array("recorder", "player", "delayerd")) ||
			!in_array($command, array("start", "stop", "restart")))
		{
			json_response(422, array("ok" => false, "error" => "invalid_process_command"));
		}
		if (!preg_match('/^[A-Za-z0-9_.-]+$/', $interface))
		{
			json_response(422, array("ok" => false, "error" => "invalid_interface_name"));
		}

		$unit = $process . "@" . $interface . ".service";
		exec("systemctl " . escapeshellarg($command) . " " . escapeshellarg($unit) . " 2>&1", $output, $ret);
		if ($ret !== 0)
		{
			json_response(500, array("ok" => false, "error" => "systemctl_failed", "output" => $output));
		}
		json_response(202, array("ok" => true, "unit" => $unit, "command" => $command));
	}

	try
	{
		$DB = DBConnect();
		authenticate($DB);
		$body = request_body();
		$action = optional_value($body, "action", optional_value($_GET, "action", ""));

		switch ($action)
		{
			case "create_interface":
				create_interface($DB, $body);
				break;
			case "add_input":
				add_input($DB, $body);
				break;
			case "add_output":
				add_output($DB, $body);
				break;
			case "reserve_output":
				reserve_output($DB, $body);
				break;
			case "release_output":
				release_output($DB, $body);
				break;
			case "playback_command":
				playback_command($DB, $body);
				break;
			case "process_command":
				process_command($body);
				break;
			default:
				json_response(404, array("ok" => false, "error" => "unknown_action"));
		}
	}
	catch (PDOException $e)
	{
		json_response(500, array("ok" => false, "error" => "database_error", "message" => $e->getMessage()));
	}
	catch (Exception $e)
	{
		json_response(500, array("ok" => false, "error" => "server_error", "message" => $e->getMessage()));
	}
?>
