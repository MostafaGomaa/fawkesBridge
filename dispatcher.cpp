
#include "dispatecher.h"
#include "ros_proxy.h"
#include <boost/bind.hpp>
#include <exception> 
#include <boost/lexical_cast.hpp>

using namespace boost::asio;



Dispatcher::Dispatecher(unsigned short client_port, unsigned short server_port):
		acceptor_(io_service_, ip::tcp::endpoint( ip::tcp::v6(), client_port))
		,client_socket_(io_service_)
		,client_port_(client_port)
		,server_port_(server_port)
		{

		acceptor_.set_option(socket_base::reuse_address(true));
		start_accept();
		this->io_service_.run();
		
}



Dispatcher::~Dispatcher()
{
	boost::system::error_code err;
  	client_socket_.shutdown(ip::tcp::socket::shutdown_both, err);
  	client_socket_.close();
  	io_service_.stop();
  	
}
void Dispatcher::disconnect(const char *where, const char *reason){

	std::cout << "Disconnected From ";
	std::cout << *where;
	std::cout << "Coz ";
	std::cout << *reason;

  boost::system::error_code ec;
  client_socket_.shutdown(ip::tcp::socket::shutdown_both, ec);
  client_socket_.close();
}

void
Dispatcher::start_accept(){
	acceptor_.async_accept(client_socket_ ,boost::bind(&RosProxy::handle_accept, this,boost::asio::placeholders::error));
}

void 
Dispatcher::handle_accept(const boost::system::error_code &ec){
	if(!ec){

		ros_proxy_= new ros_proxy(server_port_,this);//start the server after the check that everything is alright
		
		boost::system::error_code ec;
		//intiat the handshake with the rosBridge
		boost::asio::read(client_socket_, buff_c, boost::asio::transfer_at_least(1),ec);
		
		std::string s="";  
		std::ostringstream ss;
		ss << &buff_c;
		s = ss.str();
		connected_to_rosbrindge=ros_proxy_->init_handshake(s);

		start_dispatching();
		start_accept();
		
	}
}

void
Dispatcher::start_dispatching(){
	boost::asio::async_read(client_socket_, buff_c,boost::asio::transfer_at_least(1),
			boost::bind(&RosProxy::handle_client_reads, this,
						   boost::asio::placeholders::error)
			);
}


void
Dispatcher::handle_client_reads(const boost::system::error_code &ec){

	if(!ec){

	std::string req="";  
	std::ostringstream ss;
	ss << &buff_c;
	req = ss.str();

	//ALL THE JASON STUFF
	
	ros_proxy_->write_to_server(req);

	start_dispatching();
	}else {
    disconnect("handle_client_reads", ec.message().c_str());
  }

}
  
  


// int main(){

// 	dispatcher dispatcher_=new dispatcher(8080,9090);
//}