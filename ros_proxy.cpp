
#include "ros_proxy.h"
#include <boost/bind.hpp>
#include <exception> 
#include <boost/lexical_cast.hpp>

using namespace boost::asio;



RosProxy::RosProxy(unsigned short client_port,unsigned short server_port):
		acceptor_(io_service_, ip::tcp::endpoint( ip::tcp::v6(), client_port))
		,client_socket_(io_service_)
		,server_socket_(io_service_)
		,resolver_(io_service_)
		,client_port_(client_port)
		,server_port_(server_port)
		,serverInit(false)
		{

		system("startRosbridge.sh"); 
		  
		 acceptor_.set_option(socket_base::reuse_address(true));
		start_accept();
		this->io_service_.run();
		
}



RosProxy::~RosProxy()
{
	boost::system::error_code err;
  	client_socket_.shutdown(ip::tcp::socket::shutdown_both, err);
  	client_socket_.close();
  	server_socket_.shutdown(ip::tcp::socket::shutdown_both, err);
  	server_socket_.close();
  	io_service_.stop();
  	io_service_thread_.join();
  	

}
void RosProxy::disconnect(const char *where, const char *reason){

	std::cout << "Disconnected From ";
	std::cout << *where;
	std::cout << "Coz ";
	std::cout << *reason;

  boost::system::error_code ec;
  client_socket_.shutdown(ip::tcp::socket::shutdown_both, ec);
  client_socket_.close();
  server_socket_.shutdown(ip::tcp::socket::shutdown_both, ec);
  server_socket_.close();

}

void
RosProxy::start_accept(){
	acceptor_.async_accept(client_socket_ ,boost::bind(&RosProxy::handle_accept, this,boost::asio::placeholders::error));
}

void 
RosProxy::handle_accept(const boost::system::error_code &ec){
	if(!ec){
	start_server();
	}

	start_accept();
}



void RosProxy::start_server(){

  ip::tcp::resolver::query query("localhost", boost::lexical_cast<std::string>(server_port_));
  resolver_.async_resolve(query,
			  boost::bind(&RosProxy::handle_resolve, this,
				      boost::asio::placeholders::error,
				      boost::asio::placeholders::iterator));
}
void
RosProxy::handle_resolve(const boost::system::error_code& err,
					    ip::tcp::resolver::iterator endpoint_iterator)
{
  if (! err) {
    // Attempt a connection to each endpoint in the list until we
    // successfully establish a connection.
#if BOOST_ASIO_VERSION > 100409
    boost::asio::async_connect(server_socket_, endpoint_iterator,
#else
    server_socket_.async_connect(*endpoint_iterator,
#endif
			       boost::bind(&RosProxy::handle_connect, this,
					   boost::asio::placeholders::error));
  } else {
    disconnect("handle_resolve", err.message().c_str());
  }
}
void
RosProxy::handle_connect(const boost::system::error_code &err)
{
  if (! err) {
	read_from_server_to_client();
	read_from_client_to_server();
  }
  else {
    disconnect("handle_connect", err.message().c_str());
  }
}


void
RosProxy::handle_server_reads(const boost::system::error_code &ec){


	if(!ec){
		write_to_client();
		read_from_server_to_client();
	}
	else {
    disconnect("handle_server_reads", ec.message().c_str());
  }

}

void
RosProxy::handle_client_reads(const boost::system::error_code &ec){

	if(!ec){
		write_to_server();
		read_from_client_to_server();
	}else {
    disconnect("handle_client_reads", ec.message().c_str());
  }

}



void
RosProxy::read_from_server_to_client(){

  std::cout << "Waiting to read from  server! \n";
		 boost::asio::async_read(server_socket_, buff_s, boost::asio::transfer_at_least(1),
		 	boost::bind(&RosProxy::handle_server_reads, this,
		 				   boost::asio::placeholders::error)
		 	);

}

void
RosProxy::read_from_client_to_server(){

  std::cout << "Waiting to read from  client! \n";
		boost::asio::async_read(client_socket_, buff_c,boost::asio::transfer_at_least(1),
			boost::bind(&RosProxy::handle_client_reads, this,
						   boost::asio::placeholders::error)
			);

}

void
RosProxy::write_to_server()
{
	boost::system::error_code ec;
	std::string s="";  
	std::ostringstream ss;
	ss << &buff_c;
	s = ss.str();
	size_t t =boost::asio::buffer_size(boost::asio::buffer(s));
	
	std::cout << "Writing to server!";
	std::cout << t;
	std::cout << "bytes \n!";

  boost::asio::write(server_socket_,boost::asio::buffer(s) , ec);
  if(ec){
  		std::cout << "Failed to write! \n";
  }
  
  
}
void
RosProxy::write_to_client()
{
	boost::system::error_code ec;
	std::string s="";  
	std::ostringstream ss;
	ss << &buff_s;
	s = ss.str();
	size_t t =boost::asio::buffer_size(boost::asio::buffer(s));
	

	std::cout << "Writing to client!";
	std::cout << t;
	std::cout << "bytes \n!";

 	boost::asio::write(client_socket_,boost::asio::buffer(s) , ec);
	if(ec){
	  		std::cout << "Failed to write! \n";
	  }
  
}

