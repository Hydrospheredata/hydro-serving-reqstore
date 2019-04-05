package hydrosphere.timemachine

import io.grpc.stub.StreamObserver
import io.hydrosphere.timemachine.TimeMachineClient
import org.scalatest.{FlatSpec, Matchers}
import timemachine.timeMachine.Data

import scala.concurrent.duration._
import scala.concurrent.{Await, Promise}

class TimeMachineClientSpec extends FlatSpec with Matchers with ReqtoreDockerKit {

  override val StartContainersTimeout = 120 seconds

  "reqstore" should "save messages to storage via grpc" in {

    val folderName = "default"

    val service = TimeMachineClient.client("127.0.0.1", exposedGrpcPort)

    val saved = for(_ <- Range(0, 10)) yield {


      val data = data2save()
      val id = Await.result(service.save(folderName, data, true), 10 seconds)

      id.timestamp isValidLong

      val saved = Await.result(service.get(folderName, id.timestamp, id.unique), 10 seconds)
      saved.id shouldBe(Some(id))

      saved
    }

    val listPromise = Promise[List[Data]]


    service.getRange(folderName, None, None, new StreamObserver[Data] {

      var list = List[Data]()

      override def onError(t: Throwable): Unit = {listPromise.failure(t)}

      override def onCompleted(): Unit = {listPromise.success(list)}

      override def onNext(value: Data): Unit = list = list :+ value

    })

    val list = Await.result(listPromise.future, 10 seconds)
    val idsList = list.collect{case Data(Some(id), _) => id}

    for(s <- saved){
      idsList.contains(s.id.get) shouldBe(true)
    }

  }



}