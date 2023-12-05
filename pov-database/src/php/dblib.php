<?php
		// -- Copyright Polylegio AB 2012	
		// -- All rights reserved
		
		// --- @provides PGsql Basic functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@polylegio.se>
		// --- @last-modified 2012-12-01
		
		set_include_path(get_include_path() . PATH_SEPARATOR . "/var/www" . PATH_SEPARATOR . "/srv/www/htdocs");
			
			function DBConnect(){
				if(($mode = @file_get_contents("/tmp/redundancy_heartbeat_last_mode")) === FALSE){
						try{
							$DB= new PDO("pgsql:dbname=delayerdb;host=127.0.0.1", "postgres", ""); 
						}catch(Exception $e){
							throw $e;
						}
				}
				else{
					$mode=trim($mode);
					if($mode != "slave"){
						try{
							$DB = new PDO("pgsql:dbname=delayerdb;host=127.0.0.1", "postgres", ""); 
						}catch(Exception $e){
							throw $e;
						}
					}
					else{
						require_once("redundancy/redundancyxml.php");
						$config = getRedundancyConfig();
						$ip = $config['copytoip'];
						$DB= new PDO("pgsql:dbname=delayerdb;host=$ip", "postgres", ""); 
					}
				}
//				$DB= new PDO("pgsql:dbname=delayerdb;host=192.168.1.131", "postgres", ""); 
			
			
				$DB->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
				return $DB;
			}

			function DBClose(){
				global $DB;
				unset($DB);
			}

			function DBPgsqlLastInsertId($sqlQuery, $pdoObject){
			    // Checks if query is an insert and gets table name
			    if( preg_match("/^INSERT[\t\n ]+INTO[\t\n ]+([a-z0-9\_\-]+)/is", $sqlQuery, $tablename) )
			    {
			        // Gets this table's last sequence value
			        $query = "SELECT currval('" . $tablename[1] . "_id_seq') AS last_value";

			        $temp_q_id = $pdoObject->prepare($query);
			        $temp_q_id->execute();

			        if($temp_q_id)
			        {
			            $temp_result = $temp_q_id->fetch(PDO::FETCH_ASSOC);
			            return ( $temp_result ) ? $temp_result['last_value'] : false;
			        }
			    }

			    return false;
			}

			
			function DBInsert($table, $columns, $values){
				global $DB;
				$query = "INSERT INTO $table ($columns) VALUES ($values)";
				try{
					$resulset = $DB->query($query);
				}catch(PDOException $e){
					print "DB insert error: " . $e->getMessage();
					return false;
				}
				return true;
			}

			function DBInsertAndGetID($table, $columns, $values){
				global $DB;
				try{
					$result = $DB->prepare($query = "INSERT INTO $table ($columns) VALUES ($values)");
					$result->execute();
				}catch(PDOException $e){
					print "DB error:" . $e->getMessage();
					return;
				}
				return DBPgsqlLastInsertId($query, $DB);
			}

			function DBFetch($tables, $columns, $conditions, $order){
				global $DB;
				$postcond = "";

				if(func_num_args() > 4){
					$args = func_get_args();
					foreach($args[4] as $key => $value){
						$postcond = $postcond . "$key $value ";
					}
				}

				$query = "SELECT $columns FROM $tables ";
				if($conditions != ""){
					$query = $query . "WHERE " . $conditions . " ";
				}

				if($order != ""){
					$query = $query . "ORDER BY " . $order . " ";
				}

				$query = $query . $postcond;
				$resultset = $DB->query($query);
				if($resultset == null){
					print "$query\n";
				}

				$result = array();
				while($row = $resultset->fetch(PDO::FETCH_ASSOC)){
					foreach($row as  $key => $element){
						$row[$key] = stripslashes($element);
		//				$row[$key] = $element;
					}
					$result[]=$row;
				}

				return $result;
			}


			function DBUpdate($table, $updates, $conditions){
				global $DB;
				$query = "UPDATE $table SET $updates ";
				if($conditions != ""){
					$query = $query . "WHERE $conditions";
				}

				return $DB->exec($query);
			}

			function DBDelete($table, $conditions){
				global $DB;
				if($conditions == ""){
					return;
				}

				$query = "DELETE FROM $table WHERE $conditions";
				return $DB->exec($query);
			}

			function DBFixBoolean($active){
				if($active == 't'||$active=='checked'||$active == true){
					$active = "'t'";
				}
				else{
					$active = "'f'";
				}

				return $active;
			}

			function DBSingleArr($arr){
				return sizeof($arr)>0?$arr[0]:$arr;
			}

			// -- End basic functions

		function DBFixNull($value){
			if($value == ""){
				return "null";
			}
			return $value;
		}

	
		function DBRemoveFromList($allelements, $removeelements, $cmpidx){
			$result =array();
			$diff = false;
			if(func_num_args() == 4){
				$args = func_get_args();
				$diff = true;
			}

			if(sizeof($removeelements) == 0){
				return $allelements;
			}

			foreach($allelements as $element){
				if($diff){
					if(!DBIsInList($element, $removeelements, $cmpidx,$args[3])){
						$result[] = $element;
					}
				}
				else{
					if(!DBIsInList($element, $removeelements, $cmpidx)){
						$result[] = $element;
					}
				}
			}
			return $result;
		}

		function DBIsInList($needle, $list, $cmpidx){
			if(func_num_args() == 4){
				$args = func_get_args();
				$cmpidx2 = $args[3];
			}
			else {
				$cmpidx2 = $cmpidx;
			}
			foreach($list as $element){
				if($element[$cmpidx2] == $needle[$cmpidx]){
					return true;
				}
			}
			return false;
		}

		function DBEscapeString($str){
			global $DB;
			$str = $DB->quote($str);
			if($str[0] == "'"){
				$str = substr($str,1);
				$str = substr($str,0,-1);
			}
			return $str;
		}
		// -- End basic functions
		$DB = DBConnect();
?>
