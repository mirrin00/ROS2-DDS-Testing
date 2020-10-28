//
// Created by egor on 02.04.20.
//

#include "Subscriber.h"
#include <time.h>
#include <stdlib.h>

void Subscriber::createSubscriber(int argc, ACE_TCHAR *argv[]) {
    try {
	srand (time(NULL));
        // Initialize DomainParticipantFactory
        _dpf = TheParticipantFactoryWithArgs(argc, argv);



        // Create DomainParticipant
        _participant =
                _dpf->create_participant(rand() % 214748364 + 1,
                                        PARTICIPANT_QOS_DEFAULT,
                                        DDS::DomainParticipantListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(_participant.in())) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" create_participant failed!\n")));
        }

        // Register Type (Messenger::Message)
        _ts = new Messenger::MessageTypeSupportImpl();

        if (_ts->register_type(_participant.in(), "") != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" register_type failed!\n")));
        }

        // Create Topic (Movie Discussion List)
        CORBA::String_var type_name = _ts->get_type_name();
        DDS::Topic_var topic =
                _participant->create_topic("test_topic",
                                          type_name.in(),
                                          TOPIC_QOS_DEFAULT,
                                          DDS::TopicListener::_nil(),
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(topic.in())) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" create_topic failed!\n")));
        }

        // Create Subscriber
        _subscriber = _participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                               DDS::SubscriberListener::_nil(),
                                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(_subscriber.in())) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" create_subscriber failed!\n")));
        }

        DDS::DataReaderQos reader_qos;
        _subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

        // Create DataReader
        _listener = new DataReaderListenerImpl;

        _reader = _subscriber->create_datareader(topic.in(),
                                                       reader_qos,
                                              _listener.in(),
                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (CORBA::is_nil(_reader.in())) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" create_datareader failed!\n")));
        }

        _reader_i = Messenger::MessageDataReader::_narrow(_reader);

        if (CORBA::is_nil(_reader_i.in())) {
            ACE_ERROR((LM_ERROR,
                    ACE_TEXT("ERROR: %N:%l: main() -")
                    ACE_TEXT(" _narrow failed!\n")));
        }



    } catch (const CORBA::Exception& e) {
        e._tao_print_exception("Exception caught in main():");
    }

}


short Subscriber::get_id(Messenger::Message& msg) {
    return msg.id;
};

unsigned long Subscriber::get_timestamp(Messenger::Message& msg) {
    return msg.timestamp;
};


bool Subscriber::receive(Messenger::Message& msg) {

    auto start_timestamp = std::chrono::duration_cast<std::chrono::
    nanoseconds>(std::chrono::high_resolution_clock::
                 now().time_since_epoch()).count();

    DDS::ReturnCode_t error = _reader_i->take(_messages,
                                              _info,
                                              1,
                                              DDS::ANY_SAMPLE_STATE,
                                              DDS::ANY_VIEW_STATE,
                                              DDS::ANY_INSTANCE_STATE);


    unsigned long proc_time = std::chrono::duration_cast<std::chrono::
    nanoseconds>(std::chrono::high_resolution_clock::
                 now().time_since_epoch()).count()
                              - start_timestamp;

    if (error == DDS::RETCODE_OK) {
        if (_info[0].valid_data) {

            _id++;

            write_received_msg(_messages[0], proc_time);
            msg = _messages[0];
            _reader_i->return_loan(_messages, _info);
            return true;
        }
    }
    _reader_i->return_loan(_messages, _info);
    return false;
};


bool Subscriber::receive() {
    Messenger::Message msg_mock;
    return receive(msg_mock);
};

void Subscriber::cleanUp(){

    std::cout << _id << std::endl;
    // Clean-up!
    _participant->delete_contained_entities();
    _dpf->delete_participant(_participant.in());

    TheServiceParticipant->shutdown();
};
