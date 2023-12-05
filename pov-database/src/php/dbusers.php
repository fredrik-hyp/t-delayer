<?php
		// -- Copyright Polylegio AB 2012	
		// -- All rights reserved

		// --- @provides PostgreSQL User functions
		// --- @author Fredrik Nilsson <fredrik.nilsson@polylegio.se>
		// --- @last-modified 2012-07-19

	require_once("dblib.php");
	require_once("bcrypt.php");
	$dbconn = DBConnect();

	function DBFetchUserById($id)
    {
        global $DB;
		if(!is_numeric($id))
        {
			return array();
		}
        $sth = $DB->prepare("SELECT * FROM users WHERE id=:id");
        $result = $sth->execute(array(":id" => $id));
        if($result === false)
        {
            return array();
        }
//		$result = DBFetch("users", "*", "id=$id", "");
		return $sth->fetch();
	}

	function DBFetchUserByUsername($username)
    {
        global $DB;
        
		$username = DBEscapeString($username);
        $sth = $DB->prepare("SELECT * FROM users WHERE username=:username");
        $result = $sth->execute(array(":username" => $username));
//		$result = DBFetch("users", "*", "username='$username'", "");

		return $sth->fetch();
	}

	function DBFetchUserByEmail($email)
    {
        global $DB;
        
		$email = DBEscapeString($email);
        $sth = $DB->prepare("SELECT * FROM users WHERE email=:email");
        $result = $sth->execute(array(":email" => $email));
        if($result === false)
        {
            return array();
        }
        
        $result = $sth->fetch();
//		$result = DBFetch("users", "*", "email='$email'", "");

		return $result;
	}

	function DBFetchAllUsers()
    {
        global $DB;
        $sth = $DB->prepare("SELECT * FROM users ORDER BY username");
        $result = $sth->execute(array());
        if($result === false)
        {
            return array();
        }
        
        $result = $sth->fetchAll();
//		$result = DBFetch("users", "*", "", "username");
		return $result;
	}

	function DBUpdateUser($id, $username, $password, $name, $surname, $email, $admin)
    {
        global $DB;
		$id = DBEscapeString($id);
		$username = DBEscapeString($username);
		$name = DBEscapeString($name);
		$surname = DBEscapeString($surname);
		$email = DBEscapeString($email);
		$admin = DBFixBoolean($admin);

		// $result = DBUpdate("users", "username='$username', name='$name', surname='$surname', email='$email', admin=$admin", "id=$id");
	    $sth = $DB->prepare("UPDATE users SET username = :username, name=:name, surname = :surname, email = :email, admin=:admin WHERE id=:id");
        $result = $sth->execute(array(":username" => $username, ":name" => "$name", ":surname" => $surname, ":email" => $email, ":admin" => $admin));
        
		if($password != "" && $result !== false)
        {
			$bcrypt = new Bcrypt(15);
			$password = $bcrypt->hash($password);
            $sth = $DB->prepare("UPDATE users SET password=:password WHERE id=:id");
            $sth->execute(array(":password" => $password, ":id" => $id));
//			DBUpdate("users", "password='$password'", "id=$id");
		}
        
        if($result)
        {
            return 1;
        }
        else
        {
            return 0;
        }
	}

	function DBUpdateUserWithUsernameAndPassword($id, $username, $password)
    {
		global $DB;
		if(!is_numeric($id)){
			return -1;
		}
		
		$user = DBFetchUserByUsername($username);
		if(isset($user['id']) && $user['id'] != $id){
			return -2;
		}
		
		if($password != ""){
			$bcrypt = new Bcrypt(15);
			$password = $bcrypt->hash($password);
			$sth = $DB->prepare("UPDATE users SET username=:username, password=:password WHERE id=:id");
			$result = $sth->execute(array(":id" => $id, ":username" => $username, ":password" => $password));
		}
		else{
			$sth = $DB->prepare("UPDATE users SET username=:username WHERE id=:id");
			$result = $sth->execute(array(":id" => $id, ":username" => $username));
		}
		
		if(!$result){
			return 0;
		}
		return 1;
		
	}
//DBAddUser("admin", "admin", "","","", true);
	function DBAddUser($username, $password, $name, $surname, $email, $admin)
    {
        global $DB;
		$username = DBEscapeString($username);
		$name = DBEscapeString($name);
		$surname = DBEscapeString($surname);
		$email = DBEscapeString($email);
//		$admin = DBFixBoolean($admin);

        $sth = $DB->prepare("INSERT INTO users (username, password, name, surname, email, admin) VALUES (:username, :password, :name, :surname, :email, :admin)");
        $result = $sth->execute(array(":username" => $username, ":password" => "", ":name" => $name, ":surname" => $surname, ":email" => $email, ":admin" => $admin));
        $userid = -1;
		if($result)
        {
			$userid = $DB->lastInsertId("users_id_seq");
		}
        
	//	$result = DBInsertAndGetID("users", "username, password, name, surname, email, admin", "'$username', '','$name', '$surname', '$email', $admin");
		if($password != "" && $userid > 0){
			$bcrypt = new Bcrypt(15);
			$password = $bcrypt->hash($password);
            $sth = $DB->prepare("UPDATE users SET password=:password WHERE id=:userid");
            $result = $sth->execute(array(":password" => $password, ":userid" => $userid));
//			DBUpdate("users", "password='$password'", "id=$result");
		}
	
		return $userid;
	}
	
	function DBDeleteUser($id)
    {
        global $DB;
		if(!is_numeric($id))
        {
			return -1;
		}
		$id = DBEscapeString($id);
        $sth = $DB->prepare("DELETE FROM users where id=:id");
        $result = $sth->execute(array(":id" => $id));
		if($result)
        {
			return $sth->rowCount();
		}
		return -2;
	//	$result = DBDelete("users", "id=$id");
        
	}
	
	function DBUserVerify($user, $password)
    {
		$bcrypt = new Bcrypt(15);
		return $bcrypt->verify($password, $user['password']);
	}
?>
