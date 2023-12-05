<?php
    require_once("dblib.php");
	class DBStreaminterfaceBackupSink
	{
		protected $db = null;
		public $id = null;
		public $streaminterfaceconfig_id = null;
		public $name = null;
		public $priority = null;
		public $sinktype = null;
		public $host  = null;
		public $port = null;
		public $talkbackhost = null;
		public $talkbackport = null;
		public $ttl = null;
		public $hostclientcertificate_id = null;
		public $talkbackclientcertificate_id = null;
		public $maxretrytime = null;
		public $maxretries = null;
		public $streammissfailovertimeout = null;
		public $recording_id = null;
		public $rawudp = null;
		public $maxbps =null;
		public $streaminterfacebackupsinks = null;
		
		function __construct($db, $id = null)
		{
			$this->db = $db;
			if($id != null)
			{
				$this->fetch($id);
			}
		}
		
		
		protected function copyRow($row)
		{
			foreach($row as $key => $element)
			{
				if(is_numeric($key)) continue;
				$this->${$key} = $element;
			}
		}
		
		function fromArray($arr)
		{
			if(!is_array($arr))
			{
				return false;
			}
			
			isset($arr['streaminterfaceconfig_id']) ? $this->streaminterfaceconfig_id = $arr['streaminterfaceconfig_id'] : $this->streaminterfaceconfig_id = null;
			isset($arr['name']) ? $this->name = $arr['name'] : $this->name = "";
			isset($arr['priority']) ? $this->priority = $arr['priority'] : $this->priority = 0;
			isset($arr['sinktype']) ? $this->sinktype = $arr['sinktype'] : $this->sinktype = "rudp";
			isset($arr['host']) ? $this->host = $arr['host'] : $this->host = "";
			isset($arr['port']) ? $this->port = $arr['port'] : $this->port = null;
			isset($arr['talkbackhost']) ? $this->talkbackhost = $arr['talkbackhost'] : $this->talkbackhost = "";
			isset($arr['talkbackport']) ? $this->talkbackport = $arr['talkbackport'] : $this->talkbackport = null;
			isset($arr['ttl']) ? $this->ttl = $arr['ttl'] : $this->ttl = 1;
			isset($arr['hostclientcertificate_id']) ? $this->hostclientcertificate_id = $arr['hostclientcertificate_id'] : $this->hostclientcertificate_id = null;
			isset($arr['talkbackclientcertificate_id']) ? $this->talkbackclientcertificate_id = $arr['talkbackclientcertificate_id'] : $this->talkbackclientcertificate_id = null;
			isset($arr['maxretrytime']) ? $this->maxretrytime = $arr['maxretrytime'] : $this->maxretrytime = "";
			isset($arr['maxretries']) ? $this->maxretries = $arr['maxretries'] : $this->maxretries = "";
			isset($arr['streammissfailovertimeout']) ? $this->streammissfailovertimeout = $arr['streammissfailovertimeout'] : $this->streammissfailovertimeout = "0";
			isset($arr['recording_id']) ? $this->recording_id = $arr['recording_id'] : $this->recording_id = null;
			isset($arr['rawudp']) ? $this->rawudp = $arr['rawudp'] : $this->rawudp = "f";
			isset($arr['maxbps']) ? $this->maxbps = $arr['maxbps'] : $this->maxbps = 0;
			return true;
		}
		
		function toArray()
		{
			$result = array();
			$result['id'] = $this->id;
			return $result;
		}
		
		function fetch($id)
		{
			if($id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacebackupsinks where id=:id");
			$sth->execute(array(":id" => $id));
			$row = $sth->fetch();
			if($row === false){
				return false;
			}
			
			$this->copyRow($row);

			return true;
		}
			
		function fetchByStreaminterfaceSinkId($streaminterfacesinkid)
		{
			if($streaminterfacesinkid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfacebackupsinks where streaminterfacesink_id=:streaminterfacesink_id order by priority");
			$sth->execute(array(":streaminterfacesink_id" => $streaminterfacesink_id));
			$result = $sth->fetch();
			if($result === false){
				return false;
			}
			
			$this->StreaminterfaceBackupSinks = array();
			foreach($result as $sink)
			{
				$sinknew = new DBStreaminterfaceBackupSink($this->db);
				$sinknew->fromArray($sink);
				$this->StreaminterfaceBackupSinks[] = $sinknew;
			}
			return $result;
		}
		
		function insert()
		{
			$sth = $this->db->prepare("insert into streaminterfacebackupsinks (name, priority, sinktype, host, port, talkbackhost,
																				talbackport, hostclientcertificate_id, ttl, maxbps,  talkbackclientcertificate_id,
																				maxretrytime, maxretries, streammissfailovertimeout)
																				VALUES 
																				(:name, :priority, :sourcetype, :host, :port, :talkbackhost,
																					:talkbackport, :hostclientcertificate_id, :ttl, :maxbps, :maxretrytime,
																					:maxretries, :streammissfailovertimeout)");
			$result = $sth->execute(array(":name" => $name,
			 							   	":priority" => $this->priority,
											":sinktype" => $this->sinkype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":ttl" => $this->ttl,
											":maxbps" => $this->maxbps,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout));
			if($result)
			{
				return $this->db->lastInsertId("streaminterfacebackupsinks_id_seq");
			}
			return $result;
		}
		
		function update()
		{
			if($this->id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("update streaminterfacebackupsinks set name = :name,
																		 priority = :priority, 
																		 sinktype = :sinktype,
																		 host = :host,
																		 port = :port,
																		 talkbackhost = :talkbackhost,
																		 talkbackport = :talkbackport,
																		 ttl = :ttl,
																		 maxbps = maxbps,
																		 hostclientcertificate_id = :hostclientcertificate_id,
																		 talkbackclientcertificate_id = :talkbackclientcertificate_id,
																		 maxretrytime = :maxretrytime,
																		 maxretries = :maxretries,
																		 streammissfailovertimeout = :streammissfailovertimeout,
																		 where id=:id");
			$result = $sth->execute(array(	":id" => $this->id,
											":name" => $name,
			 							   	":priority" => $this->priority,
											":sinktype" => $this->sinkype,
											":host" => $this->host,
											":port" => $this->port,
											":talkbackhost" => $this->talkbackhost,
											":talkbackport" => $this->talkbackport,
											":ttl" => $this->ttl,
											":maxbps" => $this->maxbps,
											":hostclientcertificate_id" => $this->hostclientcertificate_id,
											"talkbackclientcertificate_id" => $this->talkbackclientcertificate_id,
											":maxretrytime" => $this->maxretrytime,
											":maxretries" => $this->maxretries,
											":streammissfailovertimout" => $this->streammissfailovertimeout
										));

			if(!$result)
			{
				return false;
			}
			return true;
		}
		
		function delete()
		{
			if($this->id  < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("delete from streaminterfacebackupsinks where id=:id");
			return $sth->execute(array(":id" => $this->id));
		}
	}
	
    function DBFetchStreaminterfaceBackupSinksByStreaminterfacesinkId($streaminterfacesinkid){
        global $DB;
        
        $sth = $DB->prepare("select * from streaminterfacebackupsinks where streaminterfacesink_id=:streaminterfacesinkid order by priority");
		$sth->execute(array(":streaminterfacesinkid" => $streaminterfacesinkid));

		$result = $sth->fetchAll();
		if($result === false){
			return array();
		}
		return $result;
    }

    function DBUpdateStreaminterfaceBackupSink($id, $name, $priority, $maxretrytime, $streammissfailovertimeout, $sinktype, $host, $port, $talkbackhost, $talkbackport, $hostclientcertificateid, $talkbackclientcertificateid, $maxbps=0){
        global $DB;
        
        if(!is_numeric($streaminterfacesinkid) || trim($streaminterfacesinkid) == "" )
		{
            return -1;
        }
        
        if(trim($hostclientcertificateid) == "")
		{
            $hostclientcertificateid = "null";
        }
        
        if(trim($talkbackclientcertificateid) == "")
		{
            $talkbackclientcertificateid = "null";
        }
        
        if(!is_numeric($port) || trim($port) == "")
		{
            $port = 0;
        }
        
        if(!is_numeric($talkbackport) || trim($talkbackport) == "")
		{
            $talkbackport=0;
        }
        
        if(!is_numeric($maxretrytime) || trim($maxretrytime) == "")
		{
            $maxretrytime =0;
        }
        
        if(!is_numeric($streammissfailovertimeout) || trim($streammissfailovertimeout) == "")
		{
            $streammissfailovertimeout = 0;
        }
        if(trim($hostclientcertificateid) == "" || $hostclientcertificateid == "null")
		{
            $hostclientcertificateid = null;
        }
        
        if(trim($talkbackclientcertificateid) == "" || $talkbackclientcertificateid == "null")
		{
            $talkbackclientcertificateid = null;
        }
		
        $sth = $DB->prepare("update streaminterfacebackupsinks set
                                name=:name,
                                priority=:priority,
                                maxretrytime=:maxretrytime, 
                                streammissfailovertimeout=:streammissfailovertimeout,
                                sinktype=:sinktype,
                                host=:host,
                                port=:port,
                                talkbackhost=:talkbackhost,
                                talkbackport=:talkbackport,
                                hostclientcertificate_id=:hostclientcertificateid,
                                talkbackclientcertificate_id=:talkbackclientcertificateid,
								maxbps=:maxbps,
                                where id=:id");
                                
        $result = $sth->execute(array(":name" => $name,
                        ":priority" => $priority,
                        ":maxretrytime" => $maxretrytime,
                        ":streammissfailovertimeout" => $streammissfailovertimeout,
                        ":sinktype" => $sinktype,
                        ":host" => $host,
                        ":port" => $port,
                        ":talkbackhost" => $talkbackhost,
                        ":talkbackport" => $talkbackport,
                        ":hostclientcertificateid" => $hostclientcertificateid,
                        ":talkbackclientcertificateid" => $talkbackclientcertificateid,
						":maxbps" => $maxbps,
                        ":id" => $id
                    ));
                    
        if(!$result){
            return 0;
        }
        return 1;
    }

    function DBAddStreaminterfaceBackupSink($streaminterfacesinkid, $name, $priority, $maxretrytime, $streammissfailovertimeout, $sinktype, $host, $port, $talkbackhost, $talkbackport, $hostclientcertificateid, $talkbackclientcertificateid, $maxbps=0, $ttl=1, $rawudp=false){
        global $DB;
        
        if(!is_numeric($streaminterfacesinkid) || trim($streaminterfacesinkid) == "" )
		{
            return -1;
        }
        
        if(trim($hostclientcertificateid) == "")
		{
            $hostclientcertificateid = "null";
        }
        
        if(trim($talkbackclientcertificateid) == "")
		{
            $talkbackclientcertificateid = "null";
        }
        
        if(!is_numeric($port) || trim($port) == "")
		{
            $port = 0;
        }
        
        if(!is_numeric($talkbackport) || trim($talkbackport) == "")
		{
            $talkbackport = 0;
        }
        
        if(!is_numeric($maxretrytime) || trim($maxretrytime) == "")
		{
            $maxretrytime = 0;
        }
        
        if(!is_numeric($streammissfailovertimeout) || trim($streammissfailovertimeout) == "")
		{
            $streammissfailovertimeout = 0;
        }
        if(trim($hostclientcertificateid) == "" || $hostclientcertificateid == "null")
		{
            $hostclientcertificateid = null;
        }
        
        if(trim($talkbackclientcertificateid) == "" || $talkbackclientcertificateid == "null")
		{
            $talkbackclientcertificateid = null;
        }
        
		if(!is_numeric($maxbps) || trim($maxbps) == "" || $maxbps < 0 )
		{
			$maxbps = 0;
		}
		
		if(!is_numeric($ttl) || trim($ttl) == "" || $ttl < 0)
		{
			$ttl = 1;
		}
		
		if($rawudp)
		{
			$rawudp= "t";
		}
		else
		{
			$rawudp= "f";
		}
		
        $sth = $DB->prepare("insert into streaminterfacebackupsinks (streaminterfacesink_id, name, priority, maxretrytime, streammissfailovertimeout, sinktype, host, port, talkbackhost, talkbackport, hostclientcertificate_id, talkbackclientcertificate_id, maxbps, ttl, rawudp) values (:streaminterfacesinkid, :name, :priority, :maxretrytime, :streammissfailovertimeout, :sinktype, :host, :port, :talkbackhost, :talkbackport, :hostclientcertificateid, :talkbackclientcertificateid, :maxbps, :ttl, :rawudp)");
        
        $result = $sth->execute(array(":streaminterfacesinkid" => $streaminterfacesinkid,
                                    ":name" => $name,
                                    ":priority" => $priority,
                                    ":maxretrytime" => $maxretrytime,
                                    ":streammissfailovertimeout" => $streammissfailovertimeout,
                                    ":sinktype" => $sinktype,
                                    ":host" => $host,
                                    ":port" => $port,
                                    ":talkbackhost" => $talkbackhost,
                                    ":talkbackport" => $talkbackport,
                                    ":hostclientcertificateid" => $hostclientcertificateid,
                                    ":talkbackclientcertificateid" => $talkbackclientcertificateid,
									":maxbps" => $maxbps,
									":ttl" => $ttl,
									":rawudp" => $rawudp
                                ));

        if($result){
        	return $DB->lastInsertId("streaminterfacebackupsinks_id_seq");
        }
        return $result;
    }
        
    function DBDeleteStreaminterfaceBackupSinkByStreaminterfacesinkId($streaminterfacesinkid){
        global $DB;
        
        if(trim($streaminterfacesinkid) == ""){
            return -1;
        }
        
        $sth = $DB->prepare("delete from streaminterfacebackupsinks where streaminterfacesink_id=:streaminterfacesinkid");
        $result = $sth->execute(array(":streaminterfacesinkid" => $streaminterfacesinkid));
        
        if($result){
            return $sth->rowCount();
        }
        
        return -2;
    }
?>