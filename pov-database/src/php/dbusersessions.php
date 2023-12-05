<?php
		// -- Copyright Polylegio AB 2012	
		// -- All rights reserved

		// --- @provides SQLite Usersession functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@polylegio.se>
		// --- @last-modified 2012-03-28

	define('__USERSESSION_TIMELIMIT__', 3600);

	require_once("dblib.php");	
	$dbconn = DBConnect();
	
	DBCleanupUsersessions();
	
	function DBFetchUsersessions()
    {
        global $DB;
        $sth = $DB->prepare("SELECT * FROM usersessions ORDER BY started_at");
        $result = $sth->execute(array());
        if($result === false)
        {
           return array(); 
        }
        
        $result = $sth->fetchAll();
//		$result = DBFetch("usersessions","*", "", "started_at");
		return $result;
	}
	
	function DBFetchUsersessionById($id)
    {
        global $DB;
        
		if(!is_numeric($id))
        {
			return -1;
		}
		$id = DBEscapeString($id);
        $sth = $DB->prepare("SELECT * FROM usersessions WHERE id=:id");
        $result = $sth->execute(array(":id" => $id));
		$sth->fetch();
        if($result === false)
        {
            return array();
        }
//		$result = DBFetch("usersessions", "*", "id=$id", "");
		return $result;
	}
	
	function DBFetchUsersessionBySessionid($sessionid)
    {
        global $DB;
		$sessionid = DBEscapeString($sessionid);
        $sth = $DB->prepare("SELECT * FROM usersessions WHERE sessionid=:sessionid");
        $result = $sth->execute(array(":sessionid" => $sessionid));
        $result = $sth->fetch();
        if($result === false)
        {
            return array();
        }
        
        
//		$usersession = DBFetch("usersessions", "*", "sessionid='$sessionid'", "");
		return $result;
	}
	
	function DBAddUserSession($userid, $sessionid)
    {
		global $DB;
        
		if(!is_numeric($userid))
        {
			return -1;
		}
		$userid = DBEscapeString($userid);
		$sessionid = DBEscapeString($sessionid);

        $sth = $DB->prepare("INSERT INTO usersessions (user_id, sessionid, last_heard_of) VALUES (:userid, :sessionid, current_timestamp)");
        $result = $sth->execute(array(":userid" => $userid, ":sessionid" => $sessionid));
        if($result)
        {
            return $DB->lastInsertId("usersessions_id_seq");
        }
        		
//		$result = DBInsertAndGetID("usersessions", "user_id, sessionid, last_heard_of", "$userid, '$sessionid', current_timestamp");
        return -2;
	}

	function DBUpdateUsersessionLastHeardOf($id)
    {
        global $DB;
        
		if(!is_numeric($id) || trim($id) == "")
        {
			return -1;
		}
		
		$id = DBEscapeString($id);
        $sth = $DB->prepare("UPDATE usersessions SET last_heard_of=current_timestamp WHERE id=:id");
        $result = $sth->execute(array(":id" => $id));
        if($result)
        {
            return 1;
        }
        return 0;
//		$result =DBUpdate("usersessions", "last_heard_of=current_timestamp", "id=" . $id);
        //$result = DBUpdate("usersessions");
//		return $result;
	}
	
	function DBDeleteUsersession($id)
    {
		global $DB;
		if(!is_numeric($id))
        {
			return -1;
		}
		
		$id = DBEscapeString($id);
        $sth = $DB->prepare("DELETE FROM usersessions WHERE id=:id");
        $result = $sth->execute(array(":id" => $id));
        if($result)
        {
            return 1;
        }
//		$result = DBDelete("usersessions", "id=$id");
		return 0;
	}
	
	// Removes sessions longer than __USERSESSION_TIMELIMIT__
	function DBCleanupUsersessions()
    {
        global $DB;
//		$sessions = DBFetch("usersessions", "*", "unix_timestamp() > (unix_timestamp(last_heard_of) + " . __USERSESSION_TIMELIMIT__  . ")", ""); // mysql
//		$sessions = DBFetch("usersessions", "*", "now() > strftime('%s',last_heard_of, '" . __USERSESSION_TIMELIMIT__  . " seconds')", ""); // sqlite
        $sth = $DB->prepare("SELECT * FROM usersessions WHERE (extract(epoch from now()) - extract(epoch from last_heard_of)) > " .  __USERSESSION_TIMELIMIT__ );
        $result = $sth->execute(array());
        if($result === false)
        {
            return;
        }
        
        $sessions = $sth->fetchAll();
//		$sessions = DBFetch("usersessions", "*", "(extract(epoch from now()) - extract(epoch from last_heard_of)) > " .  __USERSESSION_TIMELIMIT__  , ""); // pgsql		
		foreach($sessions as $session)
        {
			DBDeleteUsersession($session['id']);
		}
	}

?>