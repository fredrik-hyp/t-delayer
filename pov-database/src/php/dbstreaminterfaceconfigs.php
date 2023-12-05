<?php

	require_once("dblib.php");
	
	class DBStreaminterfaceConfig
	{
		protected $db = null;
		public $id = null;
		public $streaminterface_id = null;
		public $typeofconfig = null;
		public $autostart = null;
		public $msbuffer = null;
		public $resendmode  = null;
		public $reordersize = null;
		public $backuptoprimary = null;
		public $mploadbalance = null;
		public $multipath = null;
		public $mcinusecheck = null;
		public $nosessionid = null;
		
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
			error_log("id is " . $row['id'], 0);
			$this->id = $row['id'];
			$this->streaminterface_id = $row['streaminterface_id'];
			$this->typeofconfig = $row['typeofconfig'];
			$this->autostart = $row['autostart'];
			$this->msbuffer = $row['msbuffer'];
			$this->resendmode  = $row['resendmode'];
			$this->reordersize = $row['reordersize'];
			$this->backuptoprimary = $row['backuptoprimary'];
			$this->mploadbalance = $row['mploadbalance'];
			$this->multipath = $row['multipath'];
			$this->mcinusecheck = $row['mcinusecheck'];
			$this->actasserver = $row['actasserver'];
		}
		
		function fromArray($arr)
		{
			if(!is_array($arr))
			{
				return false;
			}
			
			isset($arr['streaminterface_id']) ? $this->streaminterface_id = $arr['streaminterface_id'] : $this->usertext = "";
			isset($arr['typeofconfig']) ? $this->typeofconfig = $arr['typeofconfig'] : $this->typeofconfig = "stored";
			isset($arr['autostart']) ? $this->autostart = $arr['autostart'] : $this->autostart = "'f'";
			isset($arr['msbuffer']) ? $this->msbuffer = $arr['msbuffer'] : $this->msbuffer = 0;
			isset($arr['resendmode']) ? $this->resendmode = $arr['resendmode'] : $this->resendmode = "normal";
			isset($arr['reordersize']) ? $this->reordersize = $arr['reordersize'] : $this->reordersize = 32;
			isset($arr['backuptoprimary']) ? $this->backuptoprimary = $arr['backuptoprimary'] : $this->backuptoprimary = "f";
			isset($arr['mploadbalance']) ? $this->mploadbalance = $arr['mploadbalance'] : $this->mploadbalance = "f";
			isset($arr['multipath']) ? $this->multipath = $arr['multipath'] : $this->multipath = "f";
			isset($arr['mcinusecheck']) ? $this->mcinusecheck = $arr['mcinusecheck'] : $this->mcinusecheck = "f";
			isset($arr['actasserver']) ?  $this->actasserver = $arr['actasserver'] : $this->actasserver = "f";
			return true;
		}
		
		function toArray()
		{
			$result = array();
			$result['id'] = $this->id;
			//$result['streaminterface_id'] = $this->streaminterface_id;
			//$result['typeofconfig'] = $this->typeofconfig;
			$result['autostart'] = $this->autostart;
			$result['msbuffer'] = $this->msbuffer;
			// $result['resendmode'] = $this->resendmode;
			$result['reordersize'] = $this->reordersize;
			// $result['backuptoprimary'] = $this->backuptoprimary;
			$result['mploadbalance'] = $this->mploadbalance;
			$result['multipath'] = $this->multipath;
			$result['mcinusecheck'] = $this->mcinusecheck;
			return $result;
		}
		
		function fetch($id)
		{
			if($id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfaceconfigs where id=:id");
			$sth->execute(array(":id" => $id));
			$row = $sth->fetch();
			if($row === false)
			{
				error_log("could not fetch streamconfiguration with id $id.");
				return false;
			}
			
			$this->copyRow($row);

			return true;
		}
			
		function fetchByStreaminterfaceId($streaminterfaceid)
		{
			if($streaminterfaceid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select * from streaminterfaceconfigs where id=:streaminterface_id");
			$sth->execute(array(":streaminterface_id" => $streaminterfaceid));
			$result = $sth->fetch();
			if($result === false){
				return false;
			}
			
			$this->id = $result['id'];
			return $this->fromArray($result);			
		}

		function fetchModeByStreaminterfaceId($streaminterfaceid)
		{
			if($streaminterfaceid < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("select mode from streaminterfaceconfigs where id=:streaminterface_id");
			$sth->execute(array(":streaminterface_id" => $streaminterfaceid));
			$result = $sth->fetch();
			if($result === false){
				return false;
			}
			return $result['mode'];		
		}
			
		function update()
		{
			if($this->id < 1)
			{
				return false;
			}
			
			$sth = $this->db->prepare("update streaminterfaceconfigs set autostart = :autostart,
																		 msbuffer = :msbufffer,
																		 reordersize = :reordersize,
																		 backuptoprimary = :backuptoprimary,
																		 mploadbalance = :mploadbalance,
																		 multipath = :multipath,
																		 mcinusecheck = :mcinusecheck,
																		 actasserver = :actasserver,
																		 where id=:id");
			$result = $sth->execute(array(	":id" => $this->id,
											":autostart" => $this->autostart, 
											":msbuffer" => $this->msbuffer,
											":reordersize" => $this->reordersize,
											":backuptoprimary" => $this->backuptoprimary, 
											":mploadbalance" => $this->mploadbalance,
											":multipath" => $this->multipath, 
											":mcinusecheck" => $this->mcinusecheck,
											":actasserver" => $this->actasserver
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
			
			$sth = $this->db->prepare("delete from streaminterfaceconfigs where id=:id");
			$result = $sth->execute(array(":id" => $this->id));
		}
	}
	
	function DBFetchStreaminterfaceConfigByStreaminterfaceId($streaminterfaceid){
		global $DB;
		
		$sth = $DB->prepare("select * from streaminterfaceconfigs where streaminterface_id=:streaminterfaceid");
		$sth->execute(array(":streaminterfaceid" => $streaminterfaceid));
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
		
	}
	
	function DBUpdateStreaminterfaceConfig($id, $streaminterfaceid, $typeofconfig, $autostart, $msbuffer, $resendmode, $mode, $nosessionid, $multipath, $mploadbalance, $actasserver='f'){
		global $DB;
	
		$sth = $DB->prepare("update streaminterfaceconfigs set streaminterface_id=:streaminterfaceid, typeofconfig=:typeofconfig, autostart=:autostart, msbuffer=:msbuffer, resendmode=:resendmode, mode=:mode, nosessionid=:nosessionid, multipath=:multipath, mploadbalance=:mploadbalance, actasserver=:actasserver where id=:id");
		$result =  $sth->execute(array(":id" => $id,
										":streaminterfaceid" => $streaminterfaceid,
										":typeofconfig" => $typeofconfig,
										":autostart" => $autostart,
										":msbuffer" => $msbuffer,
										":resendmode" => $resendmode,
										":mode" => $mode,
                                        ":nosessionid" => $nosessionid,
										":multipath" => $multipath,
										":mploadbalance" => $mploadbalance,
										":actasserver" => $actasserver));
		
		if($result === false){
			return 0;
		}
		
		return 1;
	}
	
	function DBSetStreaminterfaceConfigMcInUseCheck($streaminterfaceid, $mcinusecheck)
	{
		global $DB;
		
		$sth = $DB->prepare("update streaminterfaceconfigs set mcinusecheck=:mcinusecheck where streaminterface_id=:streaminterfaceid");
		
		$result = $sth->execute(array("streaminterfaceid" => $streaminterfaceid,
										"mcinusecheck" => $mcinusecheck));
		
		if($result === false)
		{
				return 0;
		}			
		return 1;				
	}
	
?>
